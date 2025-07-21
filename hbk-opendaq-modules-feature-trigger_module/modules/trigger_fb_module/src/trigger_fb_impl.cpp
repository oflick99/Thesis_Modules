#include <iostream>

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>

#include "hbk/opendaq/dispatch.h"
#include "trigger_fb_module/trigger_fb_impl.h"



BEGIN_NAMESPACE_TRIGGER_FB_MODULE

TriggerFbImpl::TriggerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , m_enableEdgeLastValue(0.0)
    , m_disableEdgeLastValue(0.0)
{
    initComponentStatus();

    m_state = false;

    if (config.assigned() && config.hasProperty("UseMultiThreadedScheduler") && !config.getPropertyValue("UseMultiThreadedScheduler"))
        packetReadyNotification = PacketReadyNotification::SameThread;
    else
        packetReadyNotification = PacketReadyNotification::Scheduler;

    addTypeDefinition();
    createInputPorts();
    createSignals();
    initProperties(config);
}

void TriggerFbImpl::addTypeDefinition()
{
    context.getTypeManager().addType(EnumerationType("LogicLevelEnumType", {"<", ">="}));
    context.getTypeManager().addType(EnumerationType("LogicEdgeEnumType", {"Faling", "Rising"}));
}

void TriggerFbImpl::initProperties(const daq::PropertyObjectPtr& config)
{

    auto enableObject = PropertyObject();
    auto disableObject = PropertyObject();

    auto enableLogicSlectionDict = Dict<Int, IString>();
    auto disableLogicSlectionDict = Dict<Int, IString>();

    // Attention: Order is important for setting default values.
    TriggerLogic triggerLogicDefaultValue = TRIGGER_LOGIC_TIME_OF_DAY;
    std::string evalValue = ")";

    if (config.assigned() && config.hasProperty(TIME_OFF_DAY_ENABLE_ENABLED_STR) && config.getPropertyValue(TIME_OFF_DAY_ENABLE_ENABLED_STR))
    {
        enableObject.addProperty(ListProperty(LOGIC_TIME_OF_DATE_STR, List<IString>("10:00:00")));
        enableObject.getOnPropertyValueWrite(LOGIC_TIME_OF_DATE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };
        
        disableObject.addProperty(ListProperty(LOGIC_TIME_OF_DATE_STR, List<IString>("10:00:00")));
        disableObject.getOnPropertyValueWrite(LOGIC_TIME_OF_DATE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };
        
        triggerLogicDefaultValue = TRIGGER_LOGIC_TIME_OF_DAY;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_TIME_OF_DATE_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_TIME_OF_DATE_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_TIME_OF_DAY) + ", \%" + std::string(LOGIC_TIME_OF_DATE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(DATE_ENABLE_ENABLED_STR) && config.getPropertyValue(DATE_ENABLE_ENABLED_STR))
    {
        enableObject.addProperty(StringProperty(LOGIC_DATE_STR, "1970-01-01"));
        enableObject.getOnPropertyValueWrite(LOGIC_DATE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        disableObject.addProperty(StringProperty(LOGIC_DATE_STR, "1970-01-01"));
        disableObject.getOnPropertyValueWrite(LOGIC_DATE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        triggerLogicDefaultValue = TRIGGER_LOGIC_DATE;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_DATE_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_DATE_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_DATE) + ", \%" + std::string(LOGIC_DATE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(EDGE_ENABLE_ENABLED_STR) && config.getPropertyValue(EDGE_ENABLE_ENABLED_STR))
    {
        enableObject.addProperty(StructProperty(LOGIC_EDGE_STR, Struct("LogicEdgeStructure", Dict<IString, IBaseObject>( {
            {LOGIC_EDGE_THRESHOLD_STR, 2.0},
            // To do. OneHBK app needs to support enum Types
            //{LOGIC_EDGE_LOGIC_STR, Enumeration("LogicEdgeEnumType", "Rising", context.getTypeManager())},
            {LOGIC_EDGE_LOGIC_STR, true},
                }), context.getTypeManager())));
        enableObject.getOnPropertyValueWrite(LOGIC_EDGE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

        disableObject.addProperty(StructProperty(LOGIC_EDGE_STR, Struct("LogicEdgeStructure", Dict<IString, IBaseObject>( {
            {LOGIC_EDGE_THRESHOLD_STR, 2.0},
            // To do. OneHBK app needs to support enum Types
            //{LOGIC_EDGE_LOGIC_STR, Enumeration("LogicEdgeEnumType", "Rising", context.getTypeManager())},
            {LOGIC_EDGE_LOGIC_STR, false},
                }), context.getTypeManager())));
        disableObject.getOnPropertyValueWrite(LOGIC_EDGE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

        triggerLogicDefaultValue = TRIGGER_LOGIC_EDGE;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_EDGE_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_EDGE_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_EDGE) + ", \%" + std::string(LOGIC_EDGE_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(DIG_IO_ENABLE_ENABLED_STR) && config.getPropertyValue(DIG_IO_ENABLE_ENABLED_STR))
    {
        enableObject.addProperty(StructProperty(LOGIC_DIGIO_STR, Struct("LogicDigIOStructure", Dict<IString, IBaseObject>( {
            {LOGIC_DIGIO_NEGATED_STR, false}
                }), context.getTypeManager())));
        enableObject.getOnPropertyValueWrite(LOGIC_DIGIO_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        disableObject.addProperty(StructProperty(LOGIC_DIGIO_STR, Struct("LogicDigIOStructure", Dict<IString, IBaseObject>( {
            {LOGIC_DIGIO_NEGATED_STR, false}
                }), context.getTypeManager())));
        disableObject.getOnPropertyValueWrite(LOGIC_DIGIO_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        triggerLogicDefaultValue = TRIGGER_LOGIC_DIGIO;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_DIGIO_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_DIGIO_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_DIGIO) + ", \%" + std::string(LOGIC_DIGIO_STR) + evalValue;
    }
    if (config.assigned() && config.hasProperty(LEVEL_ENABLE_ENABLED_STR) && config.getPropertyValue(LEVEL_ENABLE_ENABLED_STR))
    {
        enableObject.addProperty(StructProperty(LOGIC_LEVEL_STR, Struct("LogicLevelStructure", Dict<IString, IBaseObject>( {
                                                {LOGIC_LEVEL_THRESHOLD_STR, 2.0},
                                                {LOGIC_LEVEL_LOGIC_STR, true},
                                                //{LOGIC_LEVEL_LOGIC_STR, Enumeration("LogicLevelEnumType", ">=", context.getTypeManager())}
                                                }), context.getTypeManager())));
        enableObject.getOnPropertyValueWrite(LOGIC_LEVEL_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

        disableObject.addProperty(StructProperty(LOGIC_LEVEL_STR, Struct("LogicLevelStructure", Dict<IString, IBaseObject>( {
                                                {LOGIC_LEVEL_THRESHOLD_STR, 2.0},
                                                {LOGIC_LEVEL_LOGIC_STR, false},
                                                //{LOGIC_LEVEL_LOGIC_STR, Enumeration("LogicLevelEnumType", ">=", context.getTypeManager())}
                                                }), context.getTypeManager())));
        disableObject.getOnPropertyValueWrite(LOGIC_LEVEL_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

        triggerLogicDefaultValue = TRIGGER_LOGIC_LEVEL;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_LEVEL_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_LEVEL_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_LEVEL) + ", \%" + std::string(LOGIC_LEVEL_STR) + evalValue;
    }

    // Enable Object has not Duration
    auto enableLogicSelectionProperty = SparseSelectionProperty(LOGIC_TYPE_STR, enableLogicSlectionDict, static_cast<int>(triggerLogicDefaultValue));
    enableObject.addProperty(enableLogicSelectionProperty);
    enableObject.getOnPropertyValueWrite(LOGIC_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) {propertyChanged(); };

    auto enableEvalValue = "switch($" + std::string(LOGIC_TYPE_STR) + evalValue;
    const auto enableLogicTypesReferenceProperty = ReferenceProperty(std::string(LOGIC_TYPE_STR) + "s", EvalValue(enableEvalValue));
    enableObject.addProperty(enableLogicTypesReferenceProperty);

    auto enableSettingsObject = PropertyObject();

    auto holdOffTime = IntPropertyBuilder(ENABLE_SETTINGS_HOLD_OFF_TIME_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build();
    enableSettingsObject.addProperty(holdOffTime);
    enableSettingsObject.getOnPropertyValueWrite(ENABLE_SETTINGS_HOLD_OFF_TIME_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    enableObject.addProperty(ObjectProperty(TRIGGER_SETTINGS_STR, enableSettingsObject));
    objPtr.addProperty(ObjectProperty(ENABLE_STR, enableObject));


    // Fianlizing Disable Object - it has duration

    auto duration = IntPropertyBuilder(LOGIC_DURATION_STR, 1000)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build();
    disableObject.addProperty(duration);
    disableObject.getOnPropertyValueWrite(LOGIC_DURATION_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };
    
    triggerLogicDefaultValue = TRIGGER_LOGIC_DURATION;
    disableLogicSlectionDict.set(static_cast<int>(0), LOGIC_DURATION_STR);
    evalValue = ", " + std::to_string(TRIGGER_LOGIC_DURATION) + ", \%" + std::string(LOGIC_DURATION_STR) + evalValue;


    auto disableLogicSelectionProperty = SparseSelectionProperty(LOGIC_TYPE_STR, disableLogicSlectionDict, static_cast<int>(triggerLogicDefaultValue));
    disableObject.addProperty(disableLogicSelectionProperty);
    disableObject.getOnPropertyValueWrite(LOGIC_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged();};

    auto disableEvalValue = "switch($" + std::string(LOGIC_TYPE_STR) + evalValue;
    const auto disableLogicTypesReferenceProperty = ReferenceProperty(std::string(LOGIC_TYPE_STR) + "s", EvalValue(disableEvalValue));
    disableObject.addProperty(disableLogicTypesReferenceProperty);

    auto disableSettingsObject = PropertyObject();

    auto maxWaitingTime = IntPropertyBuilder(DISABLE_MAX_WAITING_TIME_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time"))
                            .setDescription("If waiting time is > 0, then the trigger is reseted after that time.").build();
    disableSettingsObject.addProperty(maxWaitingTime);
    disableSettingsObject.getOnPropertyValueWrite(DISABLE_MAX_WAITING_TIME_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    disableObject.addProperty(ObjectProperty(TRIGGER_SETTINGS_STR, disableSettingsObject));
    objPtr.addProperty(ObjectProperty(DISABLE_STR, disableObject));

    readProperties();
}

void TriggerFbImpl::propertyChanged()
{
    readProperties();
}

void TriggerFbImpl::readProperties()
{
    std::string disableStr = DISABLE_STR;
    disableStr += ".";
    std::string enableStr = ENABLE_STR;
    enableStr += ".";

    // Logic Types
    m_enableLogicType = objPtr.getPropertyValue(enableStr + LOGIC_TYPE_STR);
    LOG_T("Properties: EnableLogicType {}", m_enableLogicType);
    m_disableLogicType = objPtr.getPropertyValue(disableStr + LOGIC_TYPE_STR);
    LOG_T("Properties: DisableLogicType {}", m_enableLogicType);

    // Level 
    StructPtr enLevelStruct = objPtr.getPropertyValue(enableStr + LOGIC_LEVEL_STR);
    m_enableThreshold = enLevelStruct.getFieldValues()[0];
    m_enableGreaterEqual = enLevelStruct.getFieldValues()[1];
    LOG_T("Properties: Enable Level Threshold is: {}", m_enableThreshold);
    StructPtr disLevelStruct = objPtr.getPropertyValue(disableStr + LOGIC_LEVEL_STR);
    m_disableThreshold = disLevelStruct.getFieldValues()[0];
    m_disableGreaterEqual = disLevelStruct.getFieldValues()[1];
    LOG_T("Properties: Disable Level Threshold is: {}", m_disableThreshold);

    //Edge
    StructPtr enEdgeStruct = objPtr.getPropertyValue(enableStr + LOGIC_EDGE_STR);
    m_enableEdgeThreshold = enLevelStruct.getFieldValues()[0];
    m_enableRising = enLevelStruct.getFieldValues()[1];
    LOG_T("Properties: Enable Edge Threshold is: {}", m_enableEdgeThreshold);
    StructPtr disEdgeStruct = objPtr.getPropertyValue(disableStr + LOGIC_EDGE_STR);
    m_disableEdgeThreshold = disLevelStruct.getFieldValues()[0];
    m_disableRising = disLevelStruct.getFieldValues()[1];
    LOG_T("Properties: Disable Edge Threshold is: {}", m_disableEdgeThreshold);

    // Duration
    m_disableDuration = objPtr.getPropertyValue(disableStr + LOGIC_DURATION_STR);
    LOG_T("Properties: Disable Duration is: {}", m_disableDuration);

    // Disable Settings
    std::string disableSettingsStr = disableStr + TRIGGER_SETTINGS_STR;
    disableSettingsStr += ".";
    m_maxWaitingTime = objPtr.getPropertyValue(disableSettingsStr + DISABLE_MAX_WAITING_TIME_STR);
    

}

FunctionBlockTypePtr TriggerFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(BoolProperty("UseMultiThreadedScheduler", true));
    defaultConfig.addProperty(BoolProperty(LEVEL_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(DIG_IO_ENABLE_ENABLED_STR, false));
    defaultConfig.addProperty(BoolProperty(EDGE_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(DATE_ENABLE_ENABLED_STR, false));
    defaultConfig.addProperty(BoolProperty(TIME_OFF_DAY_ENABLE_ENABLED_STR, false));

    return FunctionBlockType(TRIGGER_FB_MODULE_TRIGGER_STR, "Trigger", "Trigger", defaultConfig);
}

void TriggerFbImpl::configure(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor, const TriggerPortType triggerPortType)
{
    std::string portName;
    if (triggerPortType == TriggerPortType::ENABLE_PORT)
        portName = "Enable Port";
    else
        portName = "Disable Port";


    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Incomplete signal descriptors of " + portName);
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        auto inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 && inputSampleType != SampleType::Float32 && inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 && inputSampleType != SampleType::Int32 && inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 && inputSampleType != SampleType::UInt16 && inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
        {
            throw std::runtime_error("Invalid sample type");
        }

        if (triggerPortType == TriggerPortType::ENABLE_PORT)
            m_enableInputSampleType = inputSampleType;
        else
            m_disableInputSampleType = inputSampleType;


        m_outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).build();
        m_outputDomainSignal.setDescriptor(m_outputDomainDataDescriptor);
        
        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Failed to set descriptor for trigger signal: {}", e.what()));
        m_outputSignal.setDescriptor(nullptr);
    }
}

void TriggerFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    PacketPtr packet;
    const auto inputEnableConnection = m_inputEnablePort.getConnection();
    const auto inputDisableConnection = m_inputDisablePort.getConnection();

    if (inputEnableConnection.assigned())
    {
        packet = inputEnableConnection.dequeue();
        while (packet.assigned())
        {
            switch (packet.getType())
            {
                case PacketType::Event:
                    processEventPacketEnable(packet);
                    break;
    
                case PacketType::Data:
                    if (m_state == TRIGGER_INACTIVE)
                        SAMPLE_TYPE_DISPATCH(m_enableInputSampleType, processEnableDataPacket, packet);
                    break;
    
                default:
                    break;
            }
    
            packet = inputEnableConnection.dequeue();
        };

    }

    if (inputDisableConnection.assigned())
    {
        packet = inputDisableConnection.dequeue();
        while (packet.assigned())
        {
            switch (packet.getType())
            {
                case PacketType::Event:
                    processEventPacketDisable(packet);
                    break;
    
                case PacketType::Data:
                    if (m_state == TRIGGER_ACTIVE)
                        SAMPLE_TYPE_DISPATCH(m_disableInputSampleType, processDisableDataPacket, packet);
                    break;
    
                default:
                    break;
            }
    
            packet = inputEnableConnection.dequeue();
        };
    }



}

void TriggerFbImpl::processEventPacketEnable(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        if (inputDataDescriptor.assigned())
            m_inputEnableDataDescriptor = inputDataDescriptor;
        if (inputDomainDataDescriptor.assigned())
            m_inputEnableDomainDataDescriptor = inputDomainDataDescriptor;

        configure(m_inputEnableDataDescriptor, m_inputEnableDomainDataDescriptor, TriggerPortType::ENABLE_PORT);
    }
}

void TriggerFbImpl::processEventPacketDisable(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);

        if (inputDataDescriptor.assigned())
            m_inputDisableDataDescriptor = inputDataDescriptor;
        if (inputDomainDataDescriptor.assigned())
            m_inputDisableDomainDataDescriptor = inputDomainDataDescriptor;

        configure(m_inputEnableDataDescriptor, m_inputEnableDomainDataDescriptor, TriggerPortType::DISABLE_PORT);

    }
}

void TriggerFbImpl::trigger(const DataPacketPtr& inputPacket, size_t triggerIndex)
{
    // Flip state
    m_state = !m_state;

    if (m_state == true)
    {
        // Set some timers;
        auto currentTime = milliSecondsSinceEpoch();
        m_triggerDurationElapsed =  currentTime + m_disableDuration;
        m_maxWaitingTimeElapsed = currentTime + m_maxWaitingTime;
    }

    Int triggeredAt = -1;
    auto inputDomainPacket = inputPacket.getDomainPacket();

    // Get value of domain packet data at sample i (when triggered)
    auto domainDataValues = static_cast<daq::Int*>(inputDomainPacket.getData());
    triggeredAt = static_cast<daq::Int>(domainDataValues[triggerIndex]);

    // Create output domain packet
    auto outputDomainPacket = DataPacket(m_outputDomainDataDescriptor, 1);
    auto domainPacketData = static_cast<daq::Int*>(outputDomainPacket.getData());
    *domainPacketData = triggeredAt;

    // Create output data packet
    auto dataPacket = DataPacketWithDomain(outputDomainPacket, m_outputDataDescriptor, 1);
    auto packetData = static_cast<daq::Bool*>(dataPacket.getData());
    *packetData = static_cast<daq::Bool>(m_state);

    // Send packets
    m_outputDomainSignal.sendPacket(outputDomainPacket);
    m_outputSignal.sendPacket(dataPacket);
}

bool TriggerFbImpl::evalLevelTrigger(const Float& value, const bool& greaterEqual, const Float& threshold )
{
    if (greaterEqual)
    {
        if (value >= threshold)
            return true;
        }
    else
    {
        if (value < threshold)
            return true;
    }
    return false;
}

bool TriggerFbImpl::evalEdgeTrigger(const Float& lastValue, const Float& value, const bool& rising, const Float& threshold)
{
    if (rising)
    {
        if ( (lastValue < threshold) && (value >= threshold))
            return true;
    }
    else
    {
        if ( (lastValue > threshold) && (value <= threshold))
            return true;
    }
    return false;
}


template <SampleType InputSampleType>
void TriggerFbImpl::processEnableDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();


    switch(m_enableLogicType)
    {
        case TRIGGER_LOGIC_LEVEL:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                Float value = static_cast<Float>(*inputData++);
                if (evalLevelTrigger(value, m_enableGreaterEqual, m_enableThreshold))
                    trigger(packet, i);
            }
            break;
        }
        case TRIGGER_LOGIC_DIGIO:
        {
            break;
        }
        case TRIGGER_LOGIC_EDGE:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                // ToDo: Need I to store the last value always. Not covered case.
                // Trigger is active. Then during active perio last value is not save. When it becomes deactive and
                // then one sample is missed. 
                Float value = static_cast<Float>(*inputData++);
                if(evalEdgeTrigger(m_enableEdgeLastValue, value, m_enableRising, m_enableEdgeThreshold))
                    trigger(packet, i);

                m_enableEdgeLastValue = value;
            }
            break;
        }
        case TRIGGER_LOGIC_DATE:
        {
            break;
        }
        case TRIGGER_LOGIC_TIME_OF_DAY:
        {
            break;
        }
        default:
            break;
    }
    
}

template <SampleType InputSampleType>
void TriggerFbImpl::processDisableDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    switch(m_disableLogicType)
    {
        case TRIGGER_LOGIC_DURATION:
            if ( milliSecondsSinceEpoch() > (m_triggerDurationElapsed))
                trigger(packet, 0);
            break;
        case TRIGGER_LOGIC_LEVEL:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                Float value = static_cast<Float>(*inputData++);
                if (evalLevelTrigger(value, m_disableGreaterEqual, m_disableThreshold))
                    trigger(packet, i);
            }
            break;
        }
        case TRIGGER_LOGIC_DIGIO:
        {
            break;
        }
        case TRIGGER_LOGIC_EDGE:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                // ToDo: Need I to store the last value always. Not covered case.
                // Trigger is active. Then during active perio last value is not save. When it becomes deactive and
                // then one sample is missed. 
                Float value = static_cast<Float>(*inputData++);
                if(evalEdgeTrigger(m_disableEdgeLastValue, value, m_disableRising, m_disableEdgeThreshold))
                    trigger(packet, i);

                m_disableEdgeLastValue = value;
            }
            break;
        }
        case TRIGGER_LOGIC_DATE:
        {
            break;
        }
        case TRIGGER_LOGIC_TIME_OF_DAY:
        {
            break;
        }
        default:
            break;
    }

    // Max waiting time watchdog
    if ( milliSecondsSinceEpoch() > (m_maxWaitingTimeElapsed))
        trigger(packet, 0);
    
}

void TriggerFbImpl::createInputPorts()
{
    m_inputEnablePort = createAndAddInputPort("InputEnable", packetReadyNotification);
    m_inputDisablePort = createAndAddInputPort("InputDisable", packetReadyNotification);
}

void TriggerFbImpl::createSignals()
{
    m_outputDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setValueRange(Range(0, 1)).build();
    m_outputSignal = createAndAddSignal(String("output"));
    m_outputSignal.setDescriptor(m_outputDataDescriptor);

    m_outputDomainSignal = createAndAddSignal(String("output_domain"));

    m_outputSignal.setDomainSignal(m_outputDomainSignal);
}


END_NAMESPACE_TRIGGER_FB_MODULE