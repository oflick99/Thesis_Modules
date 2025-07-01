#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <advanced_recorder_module/advanced_recorder_impl.h>
#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/sie/block_writer.h>
#include <advanced_recorder_module/sie/format.h>
#include <advanced_recorder_module/sie/indexed_writer.h>
#include <advanced_recorder_module/sie/vector_io_file.h>
#include <advanced_recorder_module/sie/writer.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

FunctionBlockTypePtr AdvancedRecorderImpl::createType()
{
    return FunctionBlockType(
        TYPE_ID,
        "AdvancedRecorder",
        "HBK advanced recording functionality",
        PropertyObject());
}

AdvancedRecorderImpl::AdvancedRecorderImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config
)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
{
    this->tags.add(Tags::RECORDER);

    addInputPort();
    addProperties();
}

ErrCode AdvancedRecorderImpl::startRecording()
{
    auto lock = getRecursiveConfigLock();
    recordingActive = true;
    reconfigure();

    return OPENDAQ_SUCCESS;
}

ErrCode AdvancedRecorderImpl::stopRecording()
{
    auto lock = getRecursiveConfigLock();
    recordingActive = false;
    reconfigure();

    return OPENDAQ_SUCCESS;
}

ErrCode AdvancedRecorderImpl::getIsRecording(Bool *isRecording)
{
    OPENDAQ_PARAM_NOT_NULL(isRecording);
    
    auto lock = getRecursiveConfigLock();
    *isRecording = recordingActive;

    return OPENDAQ_SUCCESS;
}

void AdvancedRecorderImpl::onConnected(const InputPortPtr& port)
{
    auto lock = getRecursiveConfigLock();
    addInputPort();
    reconfigure();
}

void AdvancedRecorderImpl::onDisconnected(const InputPortPtr& port)
{
    auto lock = getRecursiveConfigLock();

    while (portCount >= 2)
    {
        auto ports = objPtr.template asPtr<IFunctionBlock>(true).getInputPorts();
        if (ports.getItemAt(portCount - 1).getConnection().assigned())
            break;
        if (ports.getItemAt(portCount - 2).getConnection().assigned())
            break;

        removeInputPort(ports.getItemAt(--portCount));
    }

    reconfigure();
}

void AdvancedRecorderImpl::activeChanged()
{
    if (!active)
        stopRecording();
}

void AdvancedRecorderImpl::onPacketReceived(const InputPortPtr& port)
{
    auto signal = findSignal(port);

    auto connection = port.getConnection();
    if (!connection.assigned())
        return;

    PacketPtr packet;
    while ((packet = connection.dequeue()).assigned())
        if (signal)
            signal->onPacketReceived(packet);
}

void AdvancedRecorderImpl::addProperties()
{
    objPtr.addProperty(StringProperty(Props::FILENAME, ""));
    objPtr.getOnPropertyValueWrite(Props::FILENAME) += std::bind(&AdvancedRecorderImpl::reconfigure, this);
}

void AdvancedRecorderImpl::addInputPort()
{
    createAndAddInputPort("Value" + std::to_string(++portCount), PacketReadyNotification::SameThread);
}

void AdvancedRecorderImpl::reconfigure()
{
    std::string filename = static_cast<std::string>(objPtr.getPropertyValue(Props::FILENAME));

    if (recordingActive)
    {
        // Open and initialize the output file, if we haven't already.
        if (!writer)
        {
            writer = std::make_shared<hbk::sie::writer>(
                hbk::sie::indexed_writer(
                    hbk::sie::block_writer(
                        hbk::sie::vector_io_file(
                            filename))));

            auto test = hbk::sie::xml::element("test")
                .add_attribute("id", std::to_string(0));

            std::ostringstream os;
            os << hbk::sie::PREAMBLE;
            test.serialize(os, 1);

            writer->write_metadata(os.str());
        }

        // We will update the 'signals' map by emplacing new AdvancedRecorderSignal objects for
        // newly-connected input ports, and destroying AdvancedRecorderSignal objects for ports
        // that are gone or no longer connected. Notably, we will not disturb
        // AdvancedRecorderSignal objects for input ports that have not changed.

        // Keep track of the currently-connected input ports we have seen, so we can later destroy
        // AdvancedRecorderSignal objects for ports that are gone or no longer connected.
        std::set<IInputPort *> ports;

        auto inputPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();
        for (const auto& inputPort : inputPorts)
        {
            auto connection = inputPort.getConnection();
            if (connection.assigned())
            {
                ports.emplace(inputPort.getObject());

                SignalPtr signal = connection.getSignal();

                // If we don't yet have an AdvancedRecorderSignal for this port, create one.
                auto it = signals.find(inputPort.getObject());
                if (it == signals.end())
                    signals.emplace(
                        inputPort.getObject(),
                        std::make_shared<AdvancedRecorderSignal>(signal, writer, 0));
            }
        }

        // Now make another pass, and destroy AdvancedRecorderSignal
        // objects for ports that are gone or no longer connected.
        decltype(signals)::iterator it = signals.begin();
        while (it != signals.end())
        {
            if (ports.find(it->first) == ports.end())
            {
                auto jt = it;
                ++jt;
                signals.erase(it);
                it = jt;
            }

            else
            {
                ++it;
            }
        }
    }

    else
    {
        signals.clear();
        writer.reset();
    }
}

std::shared_ptr<AdvancedRecorderSignal> AdvancedRecorderImpl::findSignal(IInputPort *port)
{
    std::shared_ptr<AdvancedRecorderSignal> signal;
    auto lock = getAcquisitionLock();
    auto it = signals.find(port);
    if (it != signals.end())
        signal = it->second;
    return signal;
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
