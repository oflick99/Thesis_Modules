#include "basic_math_module/math_sum_fb.h"
#include <opendaq/function_block_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/event_packet_params.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/reader_config_ptr.h>

BEGIN_NAMESPACE_BASIC_MATH_MODULE

using namespace daq;

MathSumFbImpl::MathSumFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , inputCount(2) // Default to 2 inputs
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initProperties();
    createReader();
}

FunctionBlockTypePtr MathSumFbImpl::CreateType()
{
    return FunctionBlockType(BASIC_MATH_MODULE_MATH_SUM_STR, 
        "Math Sum", "Sums multiple input signals with configurable number of inputs");
}

void MathSumFbImpl::initProperties()
{
    // Input count property
    const auto inputCountProp = IntProperty("InputCount", 2);
    objPtr.addProperty(inputCountProp);
    objPtr.getOnPropertyValueWrite("InputCount") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { 
            auto lock = getRecursiveConfigLock();
            size_t newInputCount = args.getValue();
            
            // Disconnect ports that will be removed
            if (newInputCount < inputCount)
            {
                for (size_t i = newInputCount; i < inputCount; ++i)
                {
                    if (i < inputPorts.size() && inputPorts[i].getConnection().assigned())
                    {
                        inputPorts[i].disconnect();
                    }
                }
            }
            
            inputCount = newInputCount;
            updateInputPorts();
            createReader();
            checkPortConnections();
        };

    // Custom output range properties
    const auto customHighValueProp = FloatProperty("CustomHighValue", 10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("CustomHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("CustomLowValue", -10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("CustomLowValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto useCustomOutputRangeProp = BoolProperty("UseCustomOutputRange", False);
    objPtr.addProperty(useCustomOutputRangeProp);
    objPtr.getOnPropertyValueWrite("UseCustomOutputRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    // Tick offset tolerance property
    const auto tickOffsetToleranceUsProp = IntProperty("TickOffsetToleranceUs", 0);
    objPtr.addProperty(tickOffsetToleranceUsProp);
    objPtr.getOnPropertyValueWrite("TickOffsetToleranceUs") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { 
            readProperties();
            createReader(); 
        };

    readProperties();
}

void MathSumFbImpl::propertyChanged(bool configure)
{
    auto lock = getRecursiveConfigLock();
    readProperties();
    if (configure)
        this->configure(nullptr, {});
}

void MathSumFbImpl::readProperties()
{
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    sumHighValue = objPtr.getPropertyValue("CustomHighValue");
    sumLowValue = objPtr.getPropertyValue("CustomLowValue");
    tickOffsetToleranceUs = std::chrono::milliseconds(objPtr.getPropertyValue("TickOffsetToleranceUs"));
}

bool MathSumFbImpl::descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor != NullDataDescriptor();
}

void MathSumFbImpl::getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    }
}

bool MathSumFbImpl::getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

bool MathSumFbImpl::getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

void MathSumFbImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock();

    SizeT cnt = reader.getAvailableCount();
    
    // Create data arrays for all inputs
    std::vector<std::unique_ptr<double[]>> inputDataArrays;
    std::vector<double*> inputDataPointers;
    
    for (size_t i = 0; i < inputCount; ++i)
    {
        inputDataArrays.emplace_back(std::make_unique<double[]>(cnt));
        inputDataPointers.push_back(inputDataArrays.back().get());
    }

    const MultiReaderStatusPtr status = reader.read(inputDataPointers.data(), &cnt);

    if (cnt > 0)
    {
        const auto sumDomainPacket = DataPacket(sumDomainSignal.getDescriptor(), cnt, status.getOffset());
        const auto sumValuePacket = DataPacketWithDomain(sumDomainPacket, sumSignal.getDescriptor(), cnt);
        double* sumValueData = static_cast<double*>(sumValuePacket.getRawData());

        // Sum all inputs
        for (size_t i = 0; i < cnt; i++)
        {
            double sum = 0.0;
            for (size_t inputIdx = 0; inputIdx < inputCount; ++inputIdx)
            {
                sum += inputDataPointers[inputIdx][i];
            }
            sumValueData[i] = sum;
        }

        sumDomainSignal.sendPacket(sumDomainPacket);
        sumSignal.sendPacket(sumValuePacket);
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        const auto eventPackets = status.getEventPackets();
        if (eventPackets.getCount() > 0)
        {
            DataDescriptorPtr domainDescriptor;
            std::vector<DataDescriptorPtr> inputDescriptors;
            bool descriptorsChanged = false;

            // Check for descriptor changes on all inputs
            for (size_t i = 0; i < inputPorts.size(); ++i)
            {
                if (eventPackets.hasKey(inputPorts[i].getGlobalId()))
                {
                    DataDescriptorPtr inputDescriptor;
                    getDataDescriptors(eventPackets.get(inputPorts[i].getGlobalId()), 
                                     inputDescriptor, domainDescriptor);
                    
                    if (descriptorNotNull(inputDescriptor))
                    {
                        if (i < inputDescriptors.size())
                            inputDescriptors[i] = inputDescriptor;
                        else
                            inputDescriptors.push_back(inputDescriptor);
                        descriptorsChanged = true;
                    }
                }
            }
                
            getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

            if (descriptorsChanged || descriptorNotNull(domainDescriptor))
                configure(domainDescriptor, inputDescriptors);
        }

        if (!status.getValid())
        {
            reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
        }
    }
}

void MathSumFbImpl::checkPortConnections() const
{
    for (const auto& port : reader.asPtr<IReaderConfig>().getInputPorts())
    {
        if (!port.getConnection().assigned())
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Port {} is not connected!", port.getLocalId()));
            return;
        }
    }
    
    setComponentStatus(ComponentStatus::Ok);
}

void MathSumFbImpl::onConnected(const InputPortPtr& inputPort)
{
    LOG_D("Math Sum FB: Input port {} connected", inputPort.getLocalId())
    checkPortConnections();
}

void MathSumFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    LOG_D("Math Sum FB: Input port {} disconnected", inputPort.getLocalId())
    checkPortConnections();
}

RangePtr MathSumFbImpl::getValueRange() const
{
    if (inputDescriptors.empty())
        return nullptr;

    Float sumHigh = 0.0;
    Float sumLow = 0.0;

    for (const auto& descriptor : inputDescriptors)
    {
        if (descriptor.assigned())
        {
            const auto inputRange = descriptor.getValueRange();
            if (inputRange.assigned())
            {
                // For addition, we sum the ranges
                sumHigh += static_cast<Float>(inputRange.getHighValue());
                sumLow += static_cast<Float>(inputRange.getLowValue());
            }
        }
    }

    return Range(sumLow, sumHigh);
}

void MathSumFbImpl::configure(const DataDescriptorPtr& domainDescriptor, const std::vector<DataDescriptorPtr>& inputDescriptors)
{
    try
    {
        if (domainDescriptor.assigned())
            this->domainDescriptor = domainDescriptor;
        
        if (!inputDescriptors.empty())
            this->inputDescriptors = inputDescriptors;

        if (this->domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }

        // Check if we have at least one valid input
        bool hasValidInput = false;
        for (const auto& descriptor : this->inputDescriptors)
        {
            if (descriptorNotNull(descriptor))
            {
                hasValidInput = true;
                break;
            }
        }

        if (!hasValidInput)
        {
            LOG_W("Math Sum FB: No valid input descriptors available yet");
            return;
        }

        const auto sumDataDescriptorBuilder =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("sum", -1, "sum", "sum"));

        RangePtr sumRange;
        if (useCustomOutputRange)
            sumRange = Range(sumLowValue, sumHighValue);
        else
            sumRange = getValueRange();

        sumDataDescriptor = sumDataDescriptorBuilder.setValueRange(sumRange).build();
        sumDomainDataDescriptor = this->domainDescriptor;

        sumSignal.setDescriptor(sumDataDescriptor);
        sumDomainSignal.setDescriptor(sumDomainDataDescriptor);

        reader.setActive(True);
        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to configure sum signal: {}", e.what()));
        reader.setActive(False);
    }
}

void MathSumFbImpl::updateInputPorts()
{
    // Disconnect and remove excess input ports
    while (inputPorts.size() > inputCount)
    {
        auto portToRemove = inputPorts.back();
        
        // Explicitly disconnect if connected
        if (portToRemove.getConnection().assigned())
        {
            LOG_D("Math Sum FB: Disconnecting port {} due to InputCount reduction", portToRemove.getLocalId());
            portToRemove.disconnect();
        }
        
        removeInputPort(portToRemove);
        inputPorts.pop_back();
        
        LOG_D("Math Sum FB: Removed port, new count: {}", inputPorts.size());
    }

    // Add missing input ports
    while (inputPorts.size() < inputCount)
    {
        const size_t index = inputPorts.size();
        const auto inputPort = createAndAddInputPort(fmt::format("Input{}", index + 1), 
                                                    PacketReadyNotification::Scheduler, nullptr, true);
        inputPorts.push_back(inputPort);
        
        LOG_D("Math Sum FB: Added port {}, new count: {}", inputPort.getLocalId(), inputPorts.size());
    }

    // Resize input descriptors vector
    inputDescriptors.resize(inputCount);
    
    LOG_D("Math Sum FB: Updated input ports, total count: {}", inputPorts.size());
}

void MathSumFbImpl::createInputPorts()
{
    inputPorts.clear();
    inputDescriptors.clear();
    
    for (size_t i = 0; i < inputCount; ++i)
    {
        const auto inputPort = createAndAddInputPort(fmt::format("Input{}", i + 1), 
                                                    PacketReadyNotification::Scheduler, nullptr, true);
        inputPorts.push_back(inputPort);
        inputDescriptors.push_back(nullptr);
    }
    
    setComponentStatusWithMessage(ComponentStatus::Warning, "Not all input ports are connected!");
}

void MathSumFbImpl::createReader()
{
    auto tolerance = SimplifiedRatio(tickOffsetToleranceUs.count(), 1'000'000);
    tolerance = tolerance.simplify();

    reader.release();

    auto builder = MultiReaderBuilder()
        .setDomainReadType(SampleType::Int64)
        .setValueReadType(SampleType::Float64)
        .setTickOffsetTolerance(tolerance);

    for (const auto& port : inputPorts)
    {
        builder.addInputPort(port);
    }

    reader = builder.build();

    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable([this, thisWeakRef = std::move(thisWeakRef)]
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            this->onDataReceived();
    });
}

void MathSumFbImpl::createSignals()
{
    sumSignal = createAndAddSignal("Sum");
    sumSignal.setName("Sum");
    sumDomainSignal = createAndAddSignal("SumDomain", nullptr, false);
    sumDomainSignal.setName("SumDomain");
    sumSignal.setDomainSignal(sumDomainSignal);
}

END_NAMESPACE_BASIC_MATH_MODULE