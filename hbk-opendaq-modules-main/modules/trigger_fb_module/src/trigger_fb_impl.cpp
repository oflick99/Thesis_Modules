#include <iostream>

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sample_type_traits.h>

#include "hbk/opendaq/dispatch.h"
#include "trigger_fb_module/trigger_fb_impl.h"



BEGIN_NAMESPACE_TRIGGER_FB_MODULE

TriggerFbImpl::TriggerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
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
            {LOGIC_EDGE_THRESHOLD_STR, 0.0},
            {LOGIC_EDGE_LOGIC_STR, Enumeration("LogicEdgeEnumType", "Rising", context.getTypeManager())},
                }), context.getTypeManager())));
        enableObject.getOnPropertyValueWrite(LOGIC_EDGE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        disableObject.addProperty(StructProperty(LOGIC_EDGE_STR, Struct("LogicEdgeStructure", Dict<IString, IBaseObject>( {
            {LOGIC_EDGE_THRESHOLD_STR, 0.0},
            {LOGIC_EDGE_LOGIC_STR, Enumeration("LogicEdgeEnumType", "Rising", context.getTypeManager())},
                }), context.getTypeManager())));
        disableObject.getOnPropertyValueWrite(LOGIC_EDGE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

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
                                            {LOGIC_LEVEL_THRESHOLD_STR, 0.0},
                                            {LOGIC_LEVEL_LOGIC_STR, Enumeration("LogicLevelEnumType", ">=", context.getTypeManager())}
                                                }), context.getTypeManager())));
        enableObject.getOnPropertyValueWrite(LOGIC_LEVEL_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        disableObject.addProperty(StructProperty(LOGIC_LEVEL_STR, Struct("LogicLevelStructure", Dict<IString, IBaseObject>( {
            {LOGIC_LEVEL_THRESHOLD_STR, 0.0},
            {LOGIC_LEVEL_LOGIC_STR, Enumeration("LogicLevelEnumType", ">=", context.getTypeManager())}
                }), context.getTypeManager())));
        disableObject.getOnPropertyValueWrite(LOGIC_LEVEL_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

        triggerLogicDefaultValue = TRIGGER_LOGIC_LEVEL;

        enableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_LEVEL_STR);
        disableLogicSlectionDict.set(static_cast<int>(triggerLogicDefaultValue), LOGIC_LEVEL_STR);

        evalValue = ", " + std::to_string(TRIGGER_LOGIC_LEVEL) + ", \%" + std::string(LOGIC_LEVEL_STR) + evalValue;
    }

    // Enable Object has not Duration
    auto enableLogicSelectionProperty = SparseSelectionProperty(LOGIC_TYPE_STR, enableLogicSlectionDict, static_cast<int>(triggerLogicDefaultValue));
    enableObject.addProperty(enableLogicSelectionProperty);
    enableObject.getOnPropertyValueWrite(LOGIC_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    auto enableEvalValue = "switch($" + std::string(LOGIC_TYPE_STR) + evalValue;
    const auto enableLogicTypesReferenceProperty = ReferenceProperty(std::string(LOGIC_TYPE_STR) + "s", EvalValue(enableEvalValue));
    enableObject.addProperty(enableLogicTypesReferenceProperty);

    auto enableSettingsObject = PropertyObject();

    auto holdOffTime = IntPropertyBuilder(ENABLE_SETTINGS_HOLD_OFF_TIME_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build();
    enableSettingsObject.addProperty(holdOffTime);
    enableSettingsObject.getOnPropertyValueWrite(ENABLE_SETTINGS_HOLD_OFF_TIME_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    enableObject.addProperty(ObjectProperty(SETTINGS_STR, enableSettingsObject));
    objPtr.addProperty(ObjectProperty(ENABLE_STR, enableObject));


    // Fianlizing Disable Object - it has duration

    auto duration = IntPropertyBuilder(LOGIC_DURATION_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time")).build();
    disableObject.addProperty(holdOffTime);
    disableObject.getOnPropertyValueWrite(LOGIC_DURATION_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };
        
    disableLogicSlectionDict.set(static_cast<int>(0), LOGIC_DURATION_STR);
    evalValue = ", " + std::to_string(TRIGGER_LOGIC_DURATION) + ", \%" + std::string(LOGIC_DURATION_STR) + evalValue;

    auto disableLogicSelectionProperty = SparseSelectionProperty(LOGIC_TYPE_STR, enableLogicSlectionDict, static_cast<int>(triggerLogicDefaultValue));
    disableObject.addProperty(enableLogicSelectionProperty);
    disableObject.getOnPropertyValueWrite(LOGIC_TYPE_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    auto disableEvalValue = "switch($" + std::string(LOGIC_TYPE_STR) + evalValue;
    const auto disableLogicTypesReferenceProperty = ReferenceProperty(std::string(LOGIC_TYPE_STR) + "s", EvalValue(enableEvalValue));
    disableObject.addProperty(disableLogicTypesReferenceProperty);

    auto disableSettingsObject = PropertyObject();

    auto maxWaitingTime = IntPropertyBuilder(DISABLE_MAX_WAITING_TIME_STR, 0)
                            .setMinValue(0)
                            .setUnit(Unit("ms", -1, "milli seconds", "time"))
                            .setDescription("If waiting time is > 0, then the trigger is reseted after that time.").build();
    disableSettingsObject.addProperty(maxWaitingTime);
    disableSettingsObject.getOnPropertyValueWrite(DISABLE_MAX_WAITING_TIME_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    // Move to trigger Gate?!
    //auto reTrigger = IntPropertyBuilder(DISABLE_RE_TRIGGER_STR, 0)
    //                        .setMinValue(0)
    //                        .setUnit(Unit("ms", -1, "milli seconds", "time"))
    //                        .setDescription("If the value is larger than 0, the post trigger phase will be prolonged if a new trigger occurs within this time frame.").build();
    //disableSettingsObject.addProperty(reTrigger);
    //disableSettingsObject.getOnPropertyValueWrite(DISABLE_RE_TRIGGER_STR) += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    disableObject.addProperty(ObjectProperty(SETTINGS_STR, disableSettingsObject));
    objPtr.addProperty(ObjectProperty(DISABLE_STR, disableObject));

    readProperties();
}

void TriggerFbImpl::propertyChanged()
{
    readProperties();
}

void TriggerFbImpl::readProperties()
{
    threshold = objPtr.getPropertyValue("Threshold");
}

FunctionBlockTypePtr TriggerFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(BoolProperty("UseMultiThreadedScheduler", true));
    defaultConfig.addProperty(BoolProperty(LEVEL_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(DIG_IO_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(EDGE_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(DATE_ENABLE_ENABLED_STR, true));
    defaultConfig.addProperty(BoolProperty(TIME_OFF_DAY_ENABLE_ENABLED_STR, true));

    return FunctionBlockType(TRIGGER_FB_MODULE_SCALING_STR, "Trigger", "Trigger", defaultConfig);
}

void TriggerFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void TriggerFbImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Incomplete signal descriptors");
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 && inputSampleType != SampleType::Float32 && inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 && inputSampleType != SampleType::Int32 && inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 && inputSampleType != SampleType::UInt16 && inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
        {
            throw std::runtime_error("Invalid sample type");
        }

        outputDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setValueRange(Range(0, 1)).build();
        outputSignal.setDescriptor(outputDataDescriptor);

        outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).build();
        outputDomainSignal.setDescriptor(outputDomainDataDescriptor);

        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Failed to set descriptor for trigger signal: {}", e.what()));
        outputSignal.setDescriptor(nullptr);
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
                    processEventPacket(packet);
                    break;
    
                case PacketType::Data:
                    SAMPLE_TYPE_DISPATCH(inputSampleType, processEnableDataPacket, packet);
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
                    processEventPacket(packet);
                    break;
    
                case PacketType::Data:
                    SAMPLE_TYPE_DISPATCH(inputSampleType, processDisableDataPacket, packet);
                    break;
    
                default:
                    break;
            }
    
            packet = inputEnableConnection.dequeue();
        };
    }



}

void TriggerFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}

void TriggerFbImpl::trigger(const DataPacketPtr& inputPacket, size_t triggerIndex)
{
    // Flip state
    m_state = !m_state;

    Int triggeredAt = -1;
    auto inputDomainPacket = inputPacket.getDomainPacket();

    // Get value of domain packet data at sample i (when triggered)
    auto domainDataValues = static_cast<daq::Int*>(inputDomainPacket.getData());
    triggeredAt = static_cast<daq::Int>(domainDataValues[triggerIndex]);

    // Create output domain packet
    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    auto domainPacketData = static_cast<daq::Int*>(outputDomainPacket.getData());
    *domainPacketData = triggeredAt;

    // Create output data packet
    auto dataPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    auto packetData = static_cast<daq::Bool*>(dataPacket.getData());
    *packetData = static_cast<daq::Bool>(m_state);

    // Send packets
    outputDomainSignal.sendPacket(outputDomainPacket);
    outputSignal.sendPacket(dataPacket);
}

template <SampleType InputSampleType>
void TriggerFbImpl::processEnableDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    if (m_state == TRIGGER_INACTIVE)
    {

    }
}

template <SampleType InputSampleType>
void TriggerFbImpl::processDisableDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    if (m_state == TRIGGER_ACTIVE)
    {
        
    }
}


void TriggerFbImpl::createInputPorts()
{
    m_inputEnablePort = createAndAddInputPort("InputEnable", packetReadyNotification);
    m_inputDisablePort = createAndAddInputPort("InputDisable", packetReadyNotification);
}

void TriggerFbImpl::createSignals()
{
    outputSignal = createAndAddSignal(String("output"));
    outputDomainSignal = createAndAddSignal(String("output_domain"));
    outputSignal.setDomainSignal(outputDomainSignal);
}


END_NAMESPACE_TRIGGER_FB_MODULE