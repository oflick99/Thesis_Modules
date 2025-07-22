#include "static_fatigue_module/sg_subtraction_fb.h"
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

BEGIN_NAMESPACE_STATIC_FATIGUE_MODULE

using namespace daq;

SgSubtractionFbImpl::SgSubtractionFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initProperties();
    createReader();
}

FunctionBlockTypePtr SgSubtractionFbImpl::CreateType()
{
    return FunctionBlockType(STATIC_FATIGUE_MODULE_SG_SUBTRACTION_STR, 
        "Strain Gauge Subtraction", "Temperature compensation by subtracting dummy DMS from measuring DMS");
}

void SgSubtractionFbImpl::initProperties()
{
    const auto measuringDmsScaleProp = FloatProperty("MeasuringDmsScale", 1.0);
    objPtr.addProperty(measuringDmsScaleProp);
    objPtr.getOnPropertyValueWrite("MeasuringDmsScale") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto measuringDmsOffsetProp = FloatProperty("MeasuringDmsOffset", 0.0);
    objPtr.addProperty(measuringDmsOffsetProp);
    objPtr.getOnPropertyValueWrite("MeasuringDmsOffset") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto dummyDmsScaleProp = FloatProperty("DummyDmsScale", 1.0);
    objPtr.addProperty(dummyDmsScaleProp);
    objPtr.getOnPropertyValueWrite("DummyDmsScale") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto dummyDmsOffsetProp = FloatProperty("DummyDmsOffset", 0.0);
    objPtr.addProperty(dummyDmsOffsetProp);
    objPtr.getOnPropertyValueWrite("DummyDmsOffset") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

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

    const auto tickOffsetToleranceUsProp = IntProperty("TickOffsetToleranceUs", 0.0);
    objPtr.addProperty(tickOffsetToleranceUsProp);
    objPtr.getOnPropertyValueWrite("TickOffsetToleranceUs") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); createReader(); };

    readProperties();
}

void SgSubtractionFbImpl::propertyChanged(bool configure)
{
    auto lock = getRecursiveConfigLock();
    readProperties();
    if (configure)
        this->configure(nullptr, nullptr, nullptr);
}

void SgSubtractionFbImpl::readProperties()
{
    measuringDmsScale = objPtr.getPropertyValue("MeasuringDmsScale");
    measuringDmsOffset = objPtr.getPropertyValue("MeasuringDmsOffset");
    dummyDmsScale = objPtr.getPropertyValue("DummyDmsScale");
    dummyDmsOffset = objPtr.getPropertyValue("DummyDmsOffset");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    compensatedHighValue = objPtr.getPropertyValue("CustomHighValue");
    compensatedLowValue = objPtr.getPropertyValue("CustomLowValue");
    tickOffsetToleranceUs = std::chrono::milliseconds(objPtr.getPropertyValue("TickOffsetToleranceUs"));
}

bool SgSubtractionFbImpl::descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor != NullDataDescriptor();
}

void SgSubtractionFbImpl::getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    }
}

bool SgSubtractionFbImpl::getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

bool SgSubtractionFbImpl::getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

void SgSubtractionFbImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock();

    SizeT cnt = reader.getAvailableCount();
    const auto measuringData = std::make_unique<double[]>(cnt);
    const auto dummyData = std::make_unique<double[]>(cnt);
    std::array<double*, 2> data{measuringData.get(), dummyData.get()};

    const MultiReaderStatusPtr status = reader.read(data.data(), &cnt);

    if (cnt > 0)
    {
        const auto compensatedDomainPacket = DataPacket(compensatedDomainSignal.getDescriptor(), cnt, status.getOffset());
        const auto compensatedValuePacket = DataPacketWithDomain(compensatedDomainPacket, compensatedSignal.getDescriptor(), cnt);
        double* compensatedValueData = static_cast<double*>(compensatedValuePacket.getRawData());

        // Temperature compensation: compensated = (measuring * scale + offset) - (dummy * scale + offset)
        for (size_t i = 0; i < cnt; i++)
        {
            double measuringCompensated = measuringDmsScale * measuringData[i] + measuringDmsOffset;
            double dummyCompensated = dummyDmsScale * dummyData[i] + dummyDmsOffset;
            *compensatedValueData++ = measuringCompensated - dummyCompensated;
        }

        compensatedDomainSignal.sendPacket(compensatedDomainPacket);
        compensatedSignal.sendPacket(compensatedValuePacket);
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        const auto eventPackets = status.getEventPackets();
        if (eventPackets.getCount() > 0)
        {
            DataDescriptorPtr domainDescriptor;
            DataDescriptorPtr measuringDescriptor;
            DataDescriptorPtr dummyDescriptor;

            bool domainChanged = false;
            if (eventPackets.hasKey(measuringDmsInputPort.getGlobalId()))
            {
                getDataDescriptors(eventPackets.get(measuringDmsInputPort.getGlobalId()), measuringDescriptor, domainDescriptor);
                domainChanged = descriptorNotNull(domainDescriptor);
            }

            if (eventPackets.hasKey(dummyDmsInputPort.getGlobalId()))
            {
                getDataDescriptors(eventPackets.get(dummyDmsInputPort.getGlobalId()), dummyDescriptor, domainDescriptor);
                domainChanged |= descriptorNotNull(domainDescriptor);
            }
                
            getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

            if (measuringDescriptor.assigned() || dummyDescriptor.assigned() || domainChanged)
                configure(domainDescriptor, measuringDescriptor, dummyDescriptor);
        }

        if (!status.getValid())
        {
            reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
        }
    }
}

void SgSubtractionFbImpl::checkPortConnections() const
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

void SgSubtractionFbImpl::onConnected(const InputPortPtr& inputPort)
{
    LOG_D("SG Subtraction FB: Input port {} connected", inputPort.getLocalId())
    checkPortConnections();
}

void SgSubtractionFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    LOG_D("SG Subtraction FB: Input port {} disconnected", inputPort.getLocalId())
    checkPortConnections();
}

RangePtr SgSubtractionFbImpl::getValueRange(const DataDescriptorPtr& measuringDescriptor, const DataDescriptorPtr& dummyDescriptor)
{
    const auto measuringRange = measuringDescriptor.getValueRange();
    const auto dummyRange = dummyDescriptor.getValueRange();
    if (!measuringRange.assigned() || !dummyRange.assigned())
        return nullptr;

    const Float measuringHigh = measuringRange.getHighValue();
    const Float measuringLow = measuringRange.getLowValue();
    const Float dummyHigh = dummyRange.getHighValue();
    const Float dummyLow = dummyRange.getLowValue();

    // For subtraction: max = measuringMax - dummyMin, min = measuringMin - dummyMax
    const Float compensatedHigh = measuringHigh - dummyLow;
    const Float compensatedLow = measuringLow - dummyHigh;

    return Range(compensatedLow, compensatedHigh);
}

void SgSubtractionFbImpl::configure(const DataDescriptorPtr& domainDescriptor, const DataDescriptorPtr& measuringDescriptor, const DataDescriptorPtr& dummyDescriptor)
{
    try
    {
        if (domainDescriptor.assigned())
            this->domainDescriptor = domainDescriptor;
        if (measuringDescriptor.assigned())
            this->measuringDmsDescriptor = measuringDescriptor;
        if (dummyDescriptor.assigned())
            this->dummyDmsDescriptor = dummyDescriptor;

        if (this->domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }
        if (this->measuringDmsDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input measuring DMS descriptor is not set");
        }
        if (this->dummyDmsDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input dummy DMS descriptor is not set");
        }

        // Check if both inputs have strain units (optional validation)
        if (this->measuringDmsDescriptor.assigned() && this->measuringDmsDescriptor.getUnit().assigned())
        {
            const auto unit = this->measuringDmsDescriptor.getUnit().getSymbol();
            // Common strain units: µε (micro-strain), ε (strain)
            if (unit != "µε" && unit != "ε" && unit != "με")
            {
                // Log warning but continue - unit validation is optional
                LOG_W("SG Subtraction FB: Measuring DMS unit '{}' may not be a strain unit", unit);
            }
        }

        const auto compensatedDataDescriptorBuilder =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("µε", -1, "micro-strain", "strain"));

        RangePtr compensatedRange;
        if (useCustomOutputRange)
            compensatedRange = Range(compensatedLowValue, compensatedHighValue);
        else
            compensatedRange = getValueRange(this->measuringDmsDescriptor, this->dummyDmsDescriptor);

        compensatedDataDescriptor = compensatedDataDescriptorBuilder.setValueRange(compensatedRange).build();
        compensatedDomainDataDescriptor = this->domainDescriptor;

        compensatedSignal.setDescriptor(compensatedDataDescriptor);
        compensatedDomainSignal.setDescriptor(compensatedDomainDataDescriptor);

        reader.setActive(True);
        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to set descriptor for compensated signal: {}", e.what()));
        reader.setActive(False);
    }
}

void SgSubtractionFbImpl::createInputPorts()
{
    measuringDmsInputPort = createAndAddInputPort("MeasuringDMS", PacketReadyNotification::Scheduler, nullptr, true);
    dummyDmsInputPort = createAndAddInputPort("DummyDMS", PacketReadyNotification::Scheduler, nullptr, true);
    
    setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Port {} is not connected!", measuringDmsInputPort.getLocalId()));
}

void SgSubtractionFbImpl::createReader()
{
    auto tolerance = SimplifiedRatio(tickOffsetToleranceUs.count(), 1'000'000);
    tolerance = tolerance.simplify();

    reader.release();

    reader = MultiReaderBuilder()
        .addInputPort(measuringDmsInputPort)
        .addInputPort(dummyDmsInputPort)
        .setDomainReadType(SampleType::Int64)
        .setValueReadType(SampleType::Float64)
        .setTickOffsetTolerance(tolerance)
        .build();

   // reader.setExternalListener(this->objPtr);
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable([this, thisWeakRef = std::move(thisWeakRef)]
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            this->onDataReceived();
    });
}

void SgSubtractionFbImpl::createSignals()
{
    compensatedSignal = createAndAddSignal("CompensatedStrain");
    compensatedSignal.setName("CompensatedStrain");
    compensatedDomainSignal = createAndAddSignal("CompensatedStrainDomain", nullptr, false);
    compensatedDomainSignal.setName("CompensatedStrainDomain");
    compensatedSignal.setDomainSignal(compensatedDomainSignal);
}

END_NAMESPACE_STATIC_FATIGUE_MODULE