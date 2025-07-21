#include <iostream>

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>

#include "hbk/opendaq/dispatch.h"
#include "trigger_fb_module/logic_gate_fb_impl.h"

BEGIN_NAMESPACE_TRIGGER_FB_MODULE


LogicGateFbImpl::LogicGateFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , m_portCount(0)
    , m_logicGateType(LogicGateType::LOGIC_GATE_TYPE_AND)
    , m_logicState(false)
{
    initComponentStatus();

    if (config.assigned() && config.hasProperty("UseMultiThreadedScheduler") && !config.getPropertyValue("UseMultiThreadedScheduler"))
        packetReadyNotification = PacketReadyNotification::SameThread;
    else
        packetReadyNotification = PacketReadyNotification::Scheduler;

    initProperties();
    updateInputPorts();
    createSignals();
}

void LogicGateFbImpl::initProperties()
{
    auto logicGateTypeSelection = Dict<Int, IString>();
    LogicGateType logicGateTypeDefaultValue = LogicGateType::LOGIC_GATE_TYPE_AND;
    logicGateTypeSelection.set(static_cast<int>(LogicGateType::LOGIC_GATE_TYPE_AND), LOGIC_GATE_AND_STR);
    logicGateTypeSelection.set(static_cast<int>(LogicGateType::LOGIC_GATE_TYPE_OR), LOGIC_GATE_OR_STR);
    logicGateTypeSelection.set(static_cast<int>(LogicGateType::LOGIC_GATE_TYPE_XOR), LOGIC_GATE_XOR_STR);
    
    auto logicGateTypeSelectionProperty = SparseSelectionProperty(LOGIC_GATE_TYPE_STR, logicGateTypeSelection, static_cast<int>(logicGateTypeDefaultValue));
    objPtr.addProperty(logicGateTypeSelectionProperty);
    objPtr.getOnPropertyValueWrite(LOGIC_GATE_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

void LogicGateFbImpl::propertyChanged()
{
    readProperties();
}

void LogicGateFbImpl::readProperties()
{
    m_logicGateType = objPtr.getPropertyValue(LOGIC_GATE_TYPE_STR);
    LOG_T("Properties: ResponseType {}", m_responseType)
}

FunctionBlockTypePtr LogicGateFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(BoolProperty("UseMultiThreadedScheduler", true));
    return FunctionBlockType(TRIGGER_FB_MODULE_LOGIC_GATE_STR, "LogicGate", "A logic gate for triggers.", defaultConfig);
}

void LogicGateFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    PacketPtr packet;
    
    for (auto& logicGateContext : m_logicGateContexts)
    {
        auto signalInputConnection = logicGateContext.inputPort.getConnection();
        if (signalInputConnection.assigned())
        {
            packet = signalInputConnection.dequeue();
            while (packet.assigned())
            {
                switch (packet.getType())
                {
                    case PacketType::Event:
                        processEventPacket(packet);
                        break;
                    case PacketType::Data:
                    {
                        processDataPacket(packet, logicGateContext);
                        break;
                    }
                    default:
                        break;
                }
                packet = signalInputConnection.dequeue();
            };
        }
    }
}


void LogicGateFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, "Incomplete signal descriptors of trigger signal.");
            return;
        }

        checkSignalForTriggerInputValid(inputDataDescriptor);        
    }   
}
    
void LogicGateFbImpl::checkSignalForTriggerInputValid(const DataDescriptorPtr& dataDescriptor)
{
    try
    {
        if (dataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported for Trigger Input.");
        }
        if ( dataDescriptor.getSampleType() != SampleType::UInt8 )
        {
            throw std::runtime_error("Invalid sample type for Trigger Input");
        }

        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Input Signal not allowed for TiggerInput: {}", e.what()));
    }
}


void LogicGateFbImpl::processDataPacket(const DataPacketPtr& packet, LogicGateContext& context)
{    
    bool logicState;
    auto inputData = static_cast<uint8_t*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    if (sampleCount >= 1)
    {
        if (inputData[0] > 0)
            context.active = true;
        else
            context.active = false;
    }
 

    switch (m_logicGateType)
    {
        case LogicGateType::LOGIC_GATE_TYPE_AND:
        {
            logicState = true;
            for (auto& logicGateContext : m_logicGateContexts)
            {
                if (logicGateContext.connected)
                    logicState &= logicGateContext.active;
            }
            break;
        }
        case LogicGateType::LOGIC_GATE_TYPE_OR:
        {
            logicState = false;
            for (auto& logicGateContext : m_logicGateContexts)
            {
                if (logicGateContext.connected)
                    logicState |= logicGateContext.active;
            }
            break;
        }
        case LogicGateType::LOGIC_GATE_TYPE_XOR:
        {
            logicState = false;
            size_t activeCount = 0;
            for (auto& logicGateContext : m_logicGateContexts)
            {
                if (logicGateContext.connected)
                {
                    if (logicGateContext.active)
                        activeCount++;
                }
            }
            if (activeCount == 1)
                logicState = true;
            break;
        }
        default:
            break;
    }

    if (m_logicState == logicState)
        return;
    else
        m_logicState = logicState;


    m_outputDomainDataDescriptor = DataDescriptorBuilderCopy(packet.getDomainPacket().getDataDescriptor()).setRule(ExplicitDataRule()).build();
    m_outputDomainSignal.setDescriptor(m_outputDomainDataDescriptor);

    Int triggeredAt = -1;
    auto inputDomainPacket = packet.getDomainPacket();

    // Get value of domain packet data at sample i (when triggered)
    auto domainDataValues = static_cast<daq::Int*>(inputDomainPacket.getData());
    triggeredAt = static_cast<daq::Int>(domainDataValues[0]);

    // Create output domain packet
    auto outputDomainPacket = DataPacket(m_outputDomainDataDescriptor, 1);
    auto domainPacketData = static_cast<daq::Int*>(outputDomainPacket.getData());
    *domainPacketData = triggeredAt;

    // Create output data packet
    auto dataPacket = DataPacketWithDomain(outputDomainPacket, m_outputDataDescriptor, 1);
    auto packetData = static_cast<daq::Bool*>(dataPacket.getData());
    *packetData = static_cast<daq::Bool>(m_logicState);

    // Send packets
    m_outputDomainSignal.sendPacket(outputDomainPacket);
    m_outputSignal.sendPacket(dataPacket);
    
}

void LogicGateFbImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    auto inputDataDescriptor = inputPort.getSignal().getDescriptor();
    checkSignalForTriggerInputValid(inputDataDescriptor);        

    // Adds a free input port
    updateInputPorts();
    LOG_T("Connected to port {}", inputPort.getLocalId());
}

void LogicGateFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    updateInputPorts();
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
}

void LogicGateFbImpl::updateInputPorts()
{
    for (auto it = m_logicGateContexts.begin(); it != m_logicGateContexts.end();)
    {
        if (!it->inputPort.getSignal().assigned())
        {
            removeInputPort(it->inputPort);
            it = m_logicGateContexts.erase(it);
        }
        else
        {
            it->connected = true;
            ++it;
        }
    }

    const auto inputPort = createAndAddInputPort(fmt::format("Input{}", m_portCount++), PacketReadyNotification::SameThread);

    m_logicGateContexts.emplace_back(LogicGateContext{ 0, inputPort, false, false});
    for (size_t i = 0; i < m_logicGateContexts.size(); i++)
        m_logicGateContexts[i].index = i;

}

void LogicGateFbImpl::createSignals()
{
    m_outputDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setValueRange(Range(0, 1)).build();
    m_outputSignal = createAndAddSignal(String("LogicGateOutput"));
    m_outputSignal.setDescriptor(m_outputDataDescriptor);

    m_outputDomainSignal = createAndAddSignal(String("LogicGateOutputDomain"));
    
    m_outputSignal.setDomainSignal(m_outputDomainSignal);
}

END_NAMESPACE_TRIGGER_FB_MODULE