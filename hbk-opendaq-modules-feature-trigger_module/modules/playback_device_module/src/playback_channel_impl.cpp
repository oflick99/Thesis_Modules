#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/coercer_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/procedure_factory.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/scaling_factory.h>
#include <opendaq/signal_factory.h>
#include <date/date.h>
#include <functional>
#include <boost/algorithm/string/predicate.hpp>

#include <playback_device_module/playback_channel_impl.h>
#include <playback_device_module/common.h>

BEGIN_NAMESPACE_PLAYBACK_DEVICE_MODULE

PlaybackChannelImpl::PlaybackChannelImpl(const ContextPtr& context,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const PlaybackChannelInit& init)
    : ChannelImpl(FunctionBlockType("PlaybackChannel",  fmt::format("AI{}", init.index + 1), ""), context, parent, localId)
    , index(init.index)
    , startTime(init.startTime)
    , microSecondsFromEpochToStartTime(init.microSecondsFromEpochToStartTime)
    , lastCollectTime(0)
    , playbackerenceDomainId(init.referenceDomainId)
{
    initProperties();
    fileReaderMap[".csv"] = std::bind(&PlaybackChannelImpl::openCSVSignal, this);
}

void PlaybackChannelImpl::initProperties()
{
    objPtr.addProperty(SelectionProperty("DataSource", List<IString>("Array", "File"), 0));
    objPtr.getOnPropertyValueWrite("DataSource") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { dataSourceChanged(); };

    const auto filePathProp = StringProperty("FilePath", filePath);
    objPtr.addProperty(filePathProp);
    objPtr.getOnPropertyValueWrite("FilePath") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { filePathChanged(); };

    auto valueMetaData = Dict<IString, Int>();
    valueMetaData.set("SampleRate", sampleRate);
    valueMetaData.set("Unit", valueUnit.getId());
    objPtr.addProperty(DictProperty("ValueMetaData", valueMetaData));
    objPtr.getOnPropertyValueWrite("ValueMetaData") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { };

    objPtr.addProperty(ListProperty("ValueArray", List<IFloat>(1.0, 2.0, 3.0, 4.0, 5.0)));
    objPtr.getOnPropertyValueWrite("ValueArray") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { valueDataArrayChanged(); };
    
    valueDataArrayChanged();
    dataSourceChanged();

    auto arguments = List<IArgumentInfo>(ArgumentInfo("TargetValue", ctFloat), ArgumentInfo("ReferenceValue", ctInt));
    objPtr.addProperty(FunctionProperty("Zero", FunctionInfo(ctFloat, arguments)));
    auto func = Function([&](FloatPtr targetValue = 0.0, IntegerPtr referenceValue = 0)
    {
        zeroOffset = targetValue - lastValue;
        return zeroOffset;
    });
    objPtr.setPropertyValue("Zero", func);

}

void PlaybackChannelImpl::openCSVSignal()
{
    if (file.is_open())
        file.close();

    file.open(filePath);
    if (!file.is_open())
    {
        LOG_W("Could not open file: \"{}\"", filePath);
        return;
    }

    fileDataBuffer.clear();
    std::string line;
    size_t lineCounter = 0;
    size_t cellCounter = 0;
    while (std::getline(file, line))
    {
        std::stringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ','))
        {
            // Read Signal Name
            if (lineCounter == 0)
            {
                if (cellCounter == 0)
                    timeSignalName = cell;
                else
                    valueSignalName = cell;
            }
            // Reads meta Data
            else if (lineCounter == 1)
            {
                std::stringstream cellStream(cell);
                std::string metaData;
                while (std::getline(cellStream, metaData, ';'))
                {
                    // Meta data of time Signal
                    if (cellCounter == 0)
                    {
                        if (boost::algorithm::starts_with(metaData, RESOLUTION_STR + "="))
                        {
                            std::string resolutionStr = metaData.substr(RESOLUTION_STR.size() + 1, metaData.size() - (EPOCH_STR.size() + 1));
                            auto items = splitString(resolutionStr, '/');
                            if (items.size() > 1)
                            {
                                int numirator = std::stoi(items[0]);
                                int denominator = std::stoi(items[1]);
                                resolution = (Ratio(numirator,denominator));
                            }
                        }
                        else if(boost::algorithm::starts_with(metaData, DELTAT_STR + "="))
                            deltaT = std::stoi(metaData.substr(DELTAT_STR.size() + 1, metaData.size() - (DELTAT_STR.size() + 1)));
                    }
                    // Meta Data of value Signal
                    else if (cellCounter == 1)
                    {
                        if(boost::algorithm::starts_with(metaData, UNIT_STR + "="))
                        {
                            std::string unitStr =  metaData.substr(UNIT_STR.size() + 1, metaData.size() - (UNIT_STR.size() + 1));
                            auto items = splitString(unitStr, '.');
                            if (items.size()> 3)
                            {
                                valueUnit = Unit(items[0], std::stoi(items[1]), items[2], items[3]);
                            }
                        } 
                    }                    
                }
            }
            // Read Data
            else if (lineCounter > 1)
            {
                if (cellCounter == 1)
                    fileDataBuffer.emplace_back(std::stod(cell));

            }
            ++cellCounter;
        }
        cellCounter = 0;
        ++lineCounter;
    }
    file.close();
}

void PlaybackChannelImpl::valueDataArrayChanged()
{
    ListPtr<double> list = objPtr.getPropertyValue("ValueArray");
    valueArrayBuffer.clear();
    for (auto item : list)
    {
        valueArrayBuffer.emplace_back(item);
    }
    dataSourceChanged();
}

void PlaybackChannelImpl::dataSourceChanged()
{
    // Clean Up before new playback file is read  
    removeChannelSignals();
    dataBuffer.clear();

    if (objPtr.getPropertyValue("DataSource") == 0)
    {
        if (valueArrayBuffer.empty() == false)
        {
            dataBuffer = valueArrayBuffer;
            dataBufferIndex = 0;
        }
    }
    else if (objPtr.getPropertyValue("DataSource") == 1)
    {
        if (fileDataBuffer.empty() == false)
        {
            dataBuffer = fileDataBuffer;
            dataBufferIndex = 0;
        }
    }

    // Replace meta data etc, if new data was applied
    if (dataBuffer.empty() == false)
    {
        sampleRate = (resolution.getDenominator() / deltaT) * resolution.getNumerator();
    
        auto valueMetaData = Dict<IString, Int>();
        valueMetaData.set("SampleRate", sampleRate);
        valueMetaData.set("UnitId", valueUnit.getId());
        objPtr.setPropertyValue("ValueMetaData", valueMetaData);
        createSignals();
    }
}

void PlaybackChannelImpl::filePathChanged()
{
    StringPtr filePathPtr = objPtr.getPropertyValue("FilePath");
    filePath = filePathPtr.toStdString();
    for (std::map<std::string, std::function<void(void)>>::iterator it  = fileReaderMap.begin(); it != fileReaderMap.end(); ++it)
    {
        const std::string ending = it->first;
        if (filePath.size() >= ending.size())
        {
            if (0 == filePath.compare(filePath.size() - ending.size(), ending.size(), ending))
            {   
                it->second();
                break;
            }
        }
    }
    dataSourceChanged();
}

uint64_t PlaybackChannelImpl::getSamplesSinceStart(std::chrono::microseconds time) const
{
    const uint64_t samplesSinceStart = static_cast<uint64_t>(std::trunc(static_cast<double>((time - startTime).count()) / 1'000'000.0 * sampleRate));
    return samplesSinceStart;
}

void PlaybackChannelImpl::collectSamples(std::chrono::microseconds curTime)
{
    auto lock = this->getAcquisitionLock();
    const uint64_t samplesSinceStart = getSamplesSinceStart(curTime);
    auto newSamples = samplesSinceStart - samplesGenerated;

    if (newSamples > 0)
    {
        if (valueSignal.assigned() && valueSignal.getActive())
        {
            const auto packetTime = samplesGenerated * deltaT + static_cast<uint64_t>(microSecondsFromEpochToStartTime.count());
            auto [dataPacket, domainPacket] = generateSamples(static_cast<int64_t>(packetTime), newSamples);

            valueSignal.sendPacket(std::move(dataPacket));
            timeSignal.sendPacket(std::move(domainPacket));
        }
        samplesGenerated = samplesSinceStart;
    }

    lastCollectTime = curTime;
}

std::tuple<PacketPtr, PacketPtr> PlaybackChannelImpl::generateSamples(int64_t curTime, uint64_t newSamples)
{
    auto domainPacket = DataPacket(timeSignal.getDescriptor(), newSamples, curTime);
    DataPacketPtr dataPacket;
    if (dataBuffer.empty() == false)
    {
        dataPacket = DataPacketWithDomain(domainPacket, valueSignal.getDescriptor(), newSamples);
        double* buffer = static_cast<double*>(dataPacket.getRawData());
        std::string line;
        for (size_t sampleIndex = 0; sampleIndex < newSamples; ++sampleIndex)
        {
            buffer[sampleIndex] = dataBuffer[dataBufferIndex] + zeroOffset;
            ++dataBufferIndex;
            if (dataBufferIndex >= dataBuffer.size() )
            {
                dataBufferIndex = 0;
            }
        }
        if (newSamples > 0)
            lastValue = buffer[0] - zeroOffset;
    }

    return {dataPacket, domainPacket};
}

void PlaybackChannelImpl::buildSignalDescriptors()
{
    const auto valueDescriptor = DataDescriptorBuilder()
                                 .setSampleType(SampleType::Float64)
                                 .setUnit(valueUnit)
                                 .setName(valueSignalName);

   
    valueSignal.setDescriptor(valueDescriptor.build());
    const auto timeDescriptor =
        DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setUnit(timeUnit)
            .setTickResolution(resolution)
            .setRule(LinearDataRule(deltaT, 0))
            .setOrigin(epoch)
            .setName(timeSignalName)
            .setReferenceDomainInfo(
                ReferenceDomainInfoBuilder().setReferenceDomainId(playbackerenceDomainId).setReferenceDomainOffset(0).build());

    timeSignal.setDescriptor(timeDescriptor.build());
}

void PlaybackChannelImpl::createSignals()
{
    valueSignal = createAndAddSignal(valueSignalName);
    timeSignal = createAndAddSignal(timeSignalName, nullptr, false);
    buildSignalDescriptors();
    valueSignal.setDomainSignal(timeSignal);
}

void PlaybackChannelImpl::removeChannelSignals()
{
    if (valueSignal.assigned())
    {
        removeSignal(valueSignal);
        removeSignal(timeSignal);
        valueSignal.release();
        timeSignal.release();
    }
}

void PlaybackChannelImpl::endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating)
{
    ChannelImpl<IPlaybackChannel>::endApplyProperties(propsAndValues, parentUpdating);
}

END_NAMESPACE_PLAYBACK_DEVICE_MODULE