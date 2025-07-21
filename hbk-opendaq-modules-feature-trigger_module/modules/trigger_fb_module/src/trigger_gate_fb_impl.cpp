#include <iostream>
#include <chrono>

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>

#include "hbk/opendaq/dispatch.h"
#include "trigger_fb_module/trigger_gate_fb_impl.h"

BEGIN_NAMESPACE_TRIGGER_FB_MODULE

TriggerGateFbImpl::TriggerGateFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , m_portCount(0)
    , m_gateState(GateState::OFF)
    , m_postTimeInMs(0)
    , m_retriggerTimeInMs(0)
    , m_postTriggerActive(false)
{
    initComponentStatus();

    if (config.assigned() && config.hasProperty("UseMultiThreadedScheduler") && !config.getPropertyValue("UseMultiThreadedScheduler"))
        packetReadyNotification = PacketReadyNotification::SameThread;
    else
        packetReadyNotification = PacketReadyNotification::Scheduler;

    createTriggerPort();
    updateValueInputPorts();
    initProperties(config);
}

void TriggerGateFbImpl::initProperties(const daq::PropertyObjectPtr& config)
{
    auto responseTypeSelection = Dict<Int, IString>();
    // Attention: Order is important for setting default values.
    ResponseType responseTypeDefaultValue = RESPONSE_TYPE_PRE_AND_POST;
    std::string evalValue = ")";

    if (config.assigned() && config.hasProperty(PRE_AND_POST_ENABLED_STR) && config.getPropertyValue(PRE_AND_POST_ENABLED_STR))
    {
        objPtr.addProperty(StructProperty(PRE_AND_POST_RESPONSE_TYPE_STR, Struct("PreAndPostStructure", Dict<IString, IBaseObject>( {
            {PRE_TIME_STR, 0},
            {POST_TIME_STR, 0},
                }), context.getTypeManager())));
        objPtr.getOnPropertyValueWrite(PRE_AND_POST_RESPONSE_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };
        
        responseTypeDefaultValue = RESPONSE_TYPE_PRE_AND_POST;

        responseTypeSelection.set(static_cast<int>(responseTypeDefaultValue), PRE_AND_POST_RESPONSE_TYPE_STR);

        evalValue = ", " + std::to_string(RESPONSE_TYPE_PRE_AND_POST) + ", \%" + std::string(PRE_AND_POST_RESPONSE_TYPE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(POST_ENABLED_STR) && config.getPropertyValue(POST_ENABLED_STR))
    {
        objPtr.addProperty(IntPropertyBuilder(POST_RESPONSE_TYPE_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build());
        objPtr.getOnPropertyValueWrite(POST_RESPONSE_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

        responseTypeDefaultValue = RESPONSE_TYPE_POST;

        responseTypeSelection.set(static_cast<int>(responseTypeDefaultValue), POST_RESPONSE_TYPE_STR);

        evalValue = ", " + std::to_string(RESPONSE_TYPE_POST) + ", \%" + std::string(POST_RESPONSE_TYPE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(PRE_ENABLED_STR) && config.getPropertyValue(PRE_ENABLED_STR))
    {
        objPtr.addProperty(IntPropertyBuilder(PRE_RESPONSE_TYPE_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build());
        objPtr.getOnPropertyValueWrite(PRE_RESPONSE_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        responseTypeDefaultValue = RESPONSE_TYPE_PRE;

        responseTypeSelection.set(static_cast<int>(responseTypeDefaultValue), PRE_RESPONSE_TYPE_STR);

        evalValue = ", " + std::to_string(RESPONSE_TYPE_PRE) + ", \%" + std::string(PRE_RESPONSE_TYPE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(IMMEDIATLEY_ENABLED_STR) && config.getPropertyValue(IMMEDIATLEY_ENABLED_STR))
    {
        responseTypeDefaultValue = RESPONSE_TYPE_IMMEDIATLEY;
        responseTypeSelection.set(static_cast<int>(responseTypeDefaultValue), IMMEDIATLEY_RESPONSE_TYPE_STR);
    }
    
    auto responseTypeSelectionProperty = SparseSelectionProperty(RESPONSE_TYPE_STR, responseTypeSelection, static_cast<int>(responseTypeDefaultValue));
    objPtr.addProperty(responseTypeSelectionProperty);
    objPtr.getOnPropertyValueWrite(RESPONSE_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    auto enableEvalValue = "switch($" + std::string(RESPONSE_TYPE_STR) + evalValue;
    const auto responseTypesReferenceProperty = ReferenceProperty(std::string(RESPONSE_TYPE_STR) + "s", EvalValue(enableEvalValue));
    objPtr.addProperty(responseTypesReferenceProperty);

    auto settingsObject = PropertyObject();

    auto reTrigger = IntPropertyBuilder(SETTINGS_RE_TRIGGER_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time"))
                            .setDescription("If the value is larger than 0, the post trigger phase will be prolonged if a new trigger occurs within this time frame.").build();
    settingsObject.addProperty(reTrigger);
    settingsObject.getOnPropertyValueWrite(SETTINGS_RE_TRIGGER_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };    


    objPtr.addProperty(ObjectProperty(TRIGGER_GATE_SETTINGS_STR, settingsObject));

    readProperties();
}

void TriggerGateFbImpl::propertyChanged()
{
    readProperties();
}

void TriggerGateFbImpl::readProperties()
{
    m_responseType = objPtr.getPropertyValue(RESPONSE_TYPE_STR);
    LOG_T("Properties: ResponseType {}", m_responseType)

    m_postTimeInMs = objPtr.getPropertyValue(POST_RESPONSE_TYPE_STR);
    LOG_T("Properties: POST_RESPONSE_TIME {}", postResponseTimeInSec);

    std::string retriggerPath = TRIGGER_GATE_SETTINGS_STR;
    retriggerPath += ".";
    retriggerPath += SETTINGS_RE_TRIGGER_STR;
    m_retriggerTimeInMs = objPtr.getPropertyValue(retriggerPath);
    LOG_T("Properties: RETRIGGER_TIME {}", m_retriggerTimeInMs);

}

FunctionBlockTypePtr TriggerGateFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(BoolProperty("UseMultiThreadedScheduler", true));
    defaultConfig.addProperty(BoolProperty(IMMEDIATLEY_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(PRE_ENABLED_STR, false));
    defaultConfig.addProperty(BoolProperty(POST_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(PRE_AND_POST_ENABLED_STR, false));

    return FunctionBlockType(TRIGGER_FB_MODULE_TIGGER_GATE_STR, "TriggerGate", "Trigger Gate which passes signals if trigger is active.", defaultConfig);
}


void TriggerGateFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    PacketPtr packet;
    const auto triggerInputConnection = m_triggerInput.getConnection();

    if (triggerInputConnection.assigned())
    {
        packet = triggerInputConnection.dequeue();
        while (packet.assigned())
        {
            switch (packet.getType())
            {
                case PacketType::Event:
                    processTriggerEventPacket(packet);
                    break;
                case PacketType::Data:
                    processTriggerPackage(packet);
                    break;  
                default:
                    break;
            }
    
            packet = triggerInputConnection.dequeue();
        };
    }
    
    for (auto& gateContext : m_gateContexts)
    {
        auto signalInputConnection = gateContext.inputPort.getConnection();
        if (signalInputConnection.assigned())
        {
            packet = signalInputConnection.dequeue();
            while (packet.assigned())
            {
                switch (packet.getType())
                {
                    case PacketType::Event:
                        processValueEventPacket(packet, gateContext);
                        break;
                    case PacketType::Data:
                    {
                        if (m_gateState == GateState::ACTIVE)
                        {
                            gateContext.outputSignal.sendPacket(std::move(packet));
                        }
                        else if (m_gateState == GateState::POST_ACTIVE)
                        {
                            if (m_postTimeElapsedInMs > milliSecondsSinceEpoch())
                                gateContext.outputSignal.sendPacket(std::move(packet));
                            else 
                                m_gateState = GateState::OFF; 
                        } 
                        else if (m_gateState > GateState::PRE)
                        {
                            SAMPLE_TYPE_DISPATCH(gateContext.inputSampleType, processDataPacket, packet);
                        }
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


void TriggerGateFbImpl::processTriggerEventPacket(const EventPacketPtr& packet)
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

        try
        {
            if (inputDataDescriptor.getDimensions().getCount() > 0)
            {
                throw std::runtime_error("Arrays not supported for Trigger Input.");
            }
            if ( inputDataDescriptor.getSampleType() != SampleType::UInt8 )
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
}

void TriggerGateFbImpl::processValueEventPacket(const EventPacketPtr& packet, GateContext& gateContext)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        std::string message = "Incomplete signal descriptors of value signal with index " + std::to_string(gateContext.index) + "!";
        if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, message);
            return;
        }

        gateContext.inputSampleType = inputDataDescriptor.getSampleType();
        gateContext.inputSampleDimension = inputDataDescriptor.getDimensions().getCount();
        try
        {
            gateContext.outputDataDescriptor = inputDataDescriptor;
            gateContext.outputDomainDataDescriptor = inputDomainDataDescriptor;
            gateContext.outputSignal.setDescriptor(gateContext.outputDataDescriptor);
            setComponentStatus(ComponentStatus::Ok);
        }
        catch (const std::exception& e)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Failed to set descriptor for value signal: {}", e.what()));
            gateContext.outputSignal.setDescriptor(nullptr);
        }
    }
}

void TriggerGateFbImpl::processTriggerPackage(const DataPacketPtr& packet)
{
    auto inputData = static_cast<uint8_t*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    if (sampleCount >= 1)
    {
        if (inputData[0] >= 1)
        {
            if (m_gateState == GateState::OFF)
                m_gateState = GateState::ACTIVE;
            else if (m_gateState == GateState::PRE)
                m_gateState = GateState::PRE_ACTIVE;
            else if (m_gateState == GateState::POST_ACTIVE)
                m_postTriggerActive = true;
        }
        else
        {
            if (m_gateState == GateState::ACTIVE)
            {
                if (m_responseType == ResponseType::RESPONSE_TYPE_POST || m_responseType == ResponseType::RESPONSE_TYPE_PRE_AND_POST)
                {
                    m_postTimeElapsedInMs = milliSecondsSinceEpoch() + m_postTimeInMs;
                    if (m_postTriggerActive)
                    {
                        m_postTimeElapsedInMs += m_retriggerTimeInMs;
                        m_postTriggerActive = false;
                    }
                    m_gateState = GateState::POST_ACTIVE;
                }    
                else
                    m_gateState = GateState::OFF;
            }
        }
    }

}

template <SampleType InputSampleType>
void TriggerGateFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    // Ringbuffer stuff... 
}

void TriggerGateFbImpl::createTriggerPort()
{
    m_triggerInput = createAndAddInputPort("TriggerInput", packetReadyNotification);
}

void TriggerGateFbImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    if ( inputPort.getLocalId() != "TriggerInput")
    {
        // By design always the last one is not connected to a signal.
        auto signal = createAndAddSignal(fmt::format("{}Triggered", inputPort.getSignal().getLocalId(), m_portCount++));
        signal.setDomainSignal(inputPort.getSignal().getDomainSignal());
        
        m_gateContexts.back().outputSignal = signal;

        // Adds a free input port
        updateValueInputPorts();
    }


    LOG_T("Connected to port {}", inputPort.getLocalId());
}

void TriggerGateFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    if ( inputPort.getLocalId() != "TriggerInput")
    {
        updateValueInputPorts();
    }
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
}

void TriggerGateFbImpl::updateValueInputPorts()
{
    for (auto it = m_gateContexts.begin(); it != m_gateContexts.end();)
    {
        if (!it->inputPort.getSignal().assigned())
        {
            removeInputPort(it->inputPort);
            if (it->outputSignal.assigned())
                removeSignal(it->outputSignal);
                
            it = m_gateContexts.erase(it);
        }
        else
            ++it;
    }

    const auto inputPort = createAndAddInputPort(fmt::format("Input{}", m_portCount++), PacketReadyNotification::SameThread);

    m_gateContexts.emplace_back(GateContext{ 0, inputPort});
    for (size_t i = 0; i < m_gateContexts.size(); i++)
    {
        m_gateContexts[i].index = i;
    }
}

END_NAMESPACE_TRIGGER_FB_MODULE