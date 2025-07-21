
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/filesystem.h>
#include <fmt/format.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_domain_factory.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/log_file_info_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/sync_component_private_ptr.h>
#include <playback_device_module/playback_channel_impl.h>
#include <playback_device_module/playback_device_impl.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <opendaq/thread_name.h>


BEGIN_NAMESPACE_PLAYBACK_DEVICE_MODULE

StringPtr ToIso8601(const std::chrono::system_clock::time_point& timePoint);

PlaybackDeviceImpl::PlaybackDeviceImpl(size_t id,
                             const PropertyObjectPtr& config,
                             const ContextPtr& ctx,
                             const ComponentPtr& parent,
                             const StringPtr& localId,
                             const StringPtr& name)
    : GenericDevice<>(ctx, parent, localId, nullptr, name)
    , id(id)
    , serialNumber(fmt::format("DevSer{}", id))
    , microSecondsFromEpochToDeviceStart(0)
    , acqLoopTime(0)
    , stopAcq(false)
    , logger(ctx.getLogger())
    , loggerComponent( this->logger.assigned()
                          ? this->logger.getOrAddComponent(PLAYBACK_MODULE_NAME)
                          : throw ArgumentNullException("Logger must not be null"))
    , samplesGenerated(0)
{
    if (config.assigned() && config.hasProperty("SerialNumber"))
    {
        const StringPtr serialTemp = config.getPropertyValue("SerialNumber");
        serialNumber = serialTemp.getLength() ? serialTemp : serialNumber;
    }

    if (const auto options = this->context.getModuleOptions(PLAYBACK_MODULE_NAME); options.assigned())
    {
        const StringPtr serialTemp = options.getOrDefault("SerialNumber");
        if (serialTemp.assigned() && serialTemp.getLength())
            serialNumber = serialTemp;
    }

    initIoFolder();
    initClock();
    initProperties(config);
    createSignals();
    configureTimeSignal();
    updateNumberOfChannels();
    updateAcqLoopTime();
    enableLogging();

    acqThread = std::thread{ &PlaybackDeviceImpl::acqLoop, this };
}

PlaybackDeviceImpl::~PlaybackDeviceImpl()
{
    {
        auto lock = this->getAcquisitionLock();
        stopAcq = true;
    }
    cv.notify_one();

    acqThread.join();
}

DeviceInfoPtr PlaybackDeviceImpl::CreateDeviceInfo(size_t id, const StringPtr& serialNumber)
{
    auto devInfo = DeviceInfoWithChanegableFields({"userName", "location"});
    devInfo.setName(fmt::format("Device {}", id));
    devInfo.setConnectionString(fmt::format("daqpb://device{}", id));
    devInfo.setManufacturer("openDAQ");
    devInfo.setModel("Playbackerence device");
    devInfo.setSerialNumber(serialNumber.assigned() && serialNumber.getLength() != 0 ? serialNumber : String(fmt::format("DevSer{}", id)));
    devInfo.setDeviceType(CreateType());

    std::string currentTime = ToIso8601(std::chrono::system_clock::now());
    devInfo.addProperty(StringProperty("SetupDate", currentTime));

    return devInfo;
}

DeviceTypePtr PlaybackDeviceImpl::CreateType()
{
    const auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("NumberOfChannels", 2));
    defaultConfig.addProperty(StringProperty("SerialNumber", ""));
    defaultConfig.addProperty(BoolProperty("EnableLogging", False));
    defaultConfig.addProperty(StringProperty("LoggingPath", "playback_device_simulator.log"));
    defaultConfig.addProperty(StringProperty("Name", ""));

    return DeviceType("daqpb",
                      "Playback device",
                      "Playback device",
                      "daqpb",
                      defaultConfig);
}

DeviceInfoPtr PlaybackDeviceImpl::onGetInfo()
{
    return PlaybackDeviceImpl::CreateDeviceInfo(id, serialNumber);
}

uint64_t PlaybackDeviceImpl::onGetTicksSinceOrigin()
{
    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    auto ticksSinceEpoch = microSecondsFromEpochToDeviceStart + microSecondsSinceDeviceStart;
    return static_cast<SizeT>(ticksSinceEpoch.count());
}

bool PlaybackDeviceImpl::allowAddDevicesFromModules()
{
    return true;
}

bool PlaybackDeviceImpl::allowAddFunctionBlocksFromModules()
{
    return true;
}

std::chrono::microseconds PlaybackDeviceImpl::getMicroSecondsSinceDeviceStart() const
{
    auto currentTime = std::chrono::steady_clock::now();
    auto microSecondsSinceDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
    return microSecondsSinceDeviceStart;
}

void PlaybackDeviceImpl::initClock()
{
    startTime = std::chrono::steady_clock::now();
    startTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(startTime.time_since_epoch());
    auto startAbsTime = std::chrono::system_clock::now();
    refDomainId = "openDAQ_" + serialNumber;

    microSecondsFromEpochToDeviceStart = std::chrono::duration_cast<std::chrono::microseconds>(startAbsTime.time_since_epoch());

    this->setDeviceDomain(
        DeviceDomain(Ratio(1,1000'000'000),
                     "1970-01-01T00:00:00+00:00",
                     UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build(),
                     ReferenceDomainInfoBuilder().setReferenceDomainId(refDomainId).setReferenceDomainOffset(0).build()));
}

void PlaybackDeviceImpl::initIoFolder()
{
    aiFolder = this->addIoFolder("AI", ioFolder);
}

void PlaybackDeviceImpl::acqLoop()
{
    daqNameThread("PlaybackDevice");

    using namespace std::chrono_literals;
    using milli = std::chrono::milliseconds;

    auto startLoopTime = std::chrono::high_resolution_clock::now();
    const auto loopTime = milli(acqLoopTime);

    auto lock = getUniqueLock();

    while (!stopAcq)
    {
        const auto time = std::chrono::high_resolution_clock::now();
        const auto loopDuration = std::chrono::duration_cast<milli>(time - startLoopTime);
        const auto waitTime = loopDuration.count() >= loopTime.count() ? milli(0) : milli(loopTime.count() - loopDuration.count());
        startLoopTime = time;

        cv.wait_for(lock, waitTime);
        if (!stopAcq)
        {
            const auto curTime = getMicroSecondsSinceDeviceStart();
            for (auto& ch : channels)
            {
                auto chPrivate = ch.asPtr<IPlaybackChannel>();
                chPrivate->collectSamples(curTime);
            }
            lastCollectTime = curTime;
        }
    }
}

void PlaybackDeviceImpl::initProperties(const PropertyObjectPtr& config)
{

    size_t numberOfChannels = 2;
    bool enableGlobalFileUsage = false;

    if (config.assigned())
    {
        if (config.hasProperty("NumberOfChannels"))
            numberOfChannels = config.getPropertyValue("NumberOfChannels");

        if (config.hasProperty("EnableLogging"))
            loggingEnabled = config.getPropertyValue("EnableLogging");

        if (config.hasProperty("LoggingPath"))
            loggingPath = config.getPropertyValue("LoggingPath");
    }

        
    objPtr.addProperty(BoolProperty("EnableGlobalFileUsage", enableGlobalFileUsage));

    objPtr.getOnPropertyValueWrite("EnableGlobalFileUsage") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { this->enableGlobalFileUsage(); };

    auto numberOfChannelsProp = IntPropertyBuilder("NumberOfChannels", numberOfChannels)
                                    .setMinValue(1)
                                    .setMaxValue(4096)
                                    .setVisible(EvalValue("!$EnableGlobalFileUsage"))
                                    .build();

    objPtr.addProperty(numberOfChannelsProp);
    objPtr.getOnPropertyValueWrite("NumberOfChannels") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateNumberOfChannels(); };

    const auto acqLoopTimePropInfo =
        IntPropertyBuilder("AcquisitionLoopTime", 20).setUnit(Unit("ms")).setMinValue(10).setMaxValue(1000).build();

    objPtr.addProperty(acqLoopTimePropInfo);
    objPtr.getOnPropertyValueWrite("AcquisitionLoopTime") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { updateAcqLoopTime(); };

    objPtr.addProperty(BoolProperty("EnableLogging", loggingEnabled));
    objPtr.getOnPropertyValueWrite("EnableLogging") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { this->enableLogging(); };
}

void PlaybackDeviceImpl::enableGlobalFileUsage()
{

}

void PlaybackDeviceImpl::updateNumberOfChannels()
{
    std::size_t num = objPtr.getPropertyValue("NumberOfChannels");
    LOG_I("Properties: NumberOfChannels {}", num);

    if (num < channels.size())
    {
        std::for_each(std::next(channels.begin(), num), channels.end(), [this](const ChannelPtr& ch)
            {
                removeChannel(nullptr, ch);
            });
        channels.erase(std::next(channels.begin(), num), channels.end());
    }

    auto microSecondsSinceDeviceStart = getMicroSecondsSinceDeviceStart();
    for (auto i = channels.size(); i < num; i++)
    {
        PlaybackChannelInit init{i, microSecondsSinceDeviceStart, microSecondsFromEpochToDeviceStart, localId};
        auto chLocalId = fmt::format("PlaybackCh{}", i);
        auto ch = createAndAddChannel<PlaybackChannelImpl>(aiFolder, chLocalId, init);
        channels.push_back(std::move(ch));
    }
}

void PlaybackDeviceImpl::updateAcqLoopTime()
{
    Int loopTime = objPtr.getPropertyValue("AcquisitionLoopTime");
    LOG_I("Properties: AcquisitionLoopTime {}", loopTime);

    this->acqLoopTime = static_cast<size_t>(loopTime);
}

void PlaybackDeviceImpl::enableLogging()
{
    loggingEnabled = objPtr.getPropertyValue("EnableLogging");
}

StringPtr ToIso8601(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::gmtime(&time);  // Use gmtime for UTC

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
    return oss.str();
}

ListPtr<ILogFileInfo> PlaybackDeviceImpl::onGetLogFileInfos()
{
    {
        auto lock = getAcquisitionLock();
        if (!loggingEnabled)
        {
            return List<ILogFileInfo>();
        }
    }

    fs::path path(loggingPath);
    if (!fs::exists(path))
    {
        return List<ILogFileInfo>();
    }

    SizeT size = fs::file_size(path);

    auto ftime = fs::last_write_time(path);

    // Convert to time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    auto lastModified = ToIso8601(sctp);

    auto logFileInfo = LogFileInfoBuilder().setName(path.filename().string())
                                           .setId(path.string())
                                           .setDescription("Log file for the playbackerence device")
                                           .setSize(size)
                                           .setEncoding("utf-8")
                                           .setLastModified(lastModified)
                                           .build();
    
    return List<ILogFileInfo>(logFileInfo);
}

StringPtr PlaybackDeviceImpl::onGetLog(const StringPtr& id, Int size, Int offset)
{
    {
        auto lock = getAcquisitionLock();
        if (!loggingEnabled)
            return "";

        if (id != loggingPath)
            return "";
    }

    std::ifstream file(loggingPath.toStdString(), std::ios::binary);
    if (!file.is_open())
        return "";

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (offset >= fileSize)
        return "";

    file.seekg(offset, std::ios::beg);

    if (size == -1)
        size = static_cast<Int>(static_cast<std::streamoff>(fileSize) - offset);
    else
        size = std::min(size, static_cast<Int>(static_cast<std::streamoff>(fileSize) - offset));

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    file.close();

    return String(buffer.data(), size);
}

std::set<OperationModeType> PlaybackDeviceImpl::onGetAvailableOperationModes()
{
    return {OperationModeType::Idle, OperationModeType::Operation, OperationModeType::SafeOperation};
}

void PlaybackDeviceImpl::configureTimeSignal()
{
    const auto timeDescriptor =
        DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", -1, "seconds", "time"))
            .setTickResolution(Ratio(1,1000'000'000))
            .setRule(LinearDataRule(1000, 0))
            .setOrigin("1970-01-01T00:00:00+00:00")
            .setName("Time")
            .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainId(refDomainId).setReferenceDomainOffset(0).build())
            .build();

    timeSignal.setDescriptor(timeDescriptor);
}


void PlaybackDeviceImpl::createSignals()
{
    timeSignal = createAndAddSignal("Time", nullptr, true);
    timeSignal.getTags().asPtr<ITagsPrivate>(true).add("DeviceDomain");
}

END_NAMESPACE_PLAYBACK_DEVICE_MODULE
