//#include <boost/locale.hpp>

#include <mtig710_device_module/mtig710_device_impl.h>
#include <mtig710_device_module/mtig710_channel_impl.h>

#include <coreobjects/unit_factory.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_MTIG710_DEVICE_MODULE

Mtig710DeviceImpl::Mtig710DeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : GenericDevice<>(ctx, parent, localId)
    , m_started(false)
    , m_sampleRate(10000)
    , m_samplesCaptured(0)
    , m_logger(ctx.getLogger())
    , m_loggerComponent( m_logger.assigned()
                          ? m_logger.getOrAddComponent("Mtig710DeviceModule")
                          : throw ArgumentNullException("Logger must not be null"))
{
    // time signal is owned by device, because in case of multiple channels they should share the same time signal
    m_timeSignal = createAndAddSignal("time", nullptr, false);

    initProperties();
    createMtig710Channel();

    start();
    
    // 1ms cylce rate
    this->setDeviceDomain(DeviceDomain(Ratio(1, 1000),
                                       "",
                                       UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build()));
}

Mtig710DeviceImpl::~Mtig710DeviceImpl()
{
    stop();
}

DeviceInfoPtr Mtig710DeviceImpl::CreateDeviceInfo()
{
    auto devInfo = DeviceInfo(getConnectionStringFromId(""));
    devInfo.setName("Mtig710Device");
    devInfo.setModel("123");
    devInfo.setDeviceType(createType());

    return devInfo;
}

/**
 * On get info is called during the device discovery process. All information,
 * which can be defined in the device info are visible if the client calls
 * isntande.getAvailableDevices()
 */
DeviceInfoPtr Mtig710DeviceImpl::onGetInfo()
{
    auto deviceInfo = CreateDeviceInfo();
    deviceInfo.freeze();
    return deviceInfo;
}

uint64_t Mtig710DeviceImpl::onGetTicksSinceOrigin()
{
    return 0;
}

/**
 * Defines a property and sets 
 */
void Mtig710DeviceImpl::initProperties()
{
    auto sampleRatePropInfo = IntPropertyBuilder("SampleRate", 10000).setReadOnly(true).setSuggestedValues(List<Int>(100, 1000, 10000)).build();
    objPtr.addProperty(sampleRatePropInfo);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

/**
 * Every device needs a channels, if it delivers measurement values. 
 * A channels is placed below an ioFolder.
 */
void Mtig710DeviceImpl::createMtig710Channel()
{
    m_channel = createAndAddChannel<Mtig710ChannelImpl>(ioFolder, "Channel1");
}

// Some Callback needed from a camera driver... 
static void cameraDataCallback()
{
    // addData
}

/**
 * The data is added via callback. IF you have an SDK which ready the data from another device out, you need to define how the add Data pacakge it calles.
 * Maybe a thread is needed.
 */
void Mtig710DeviceImpl::addData(const void* data, size_t sampleCount)
{
    try
    {
        auto domainPacket = DataPacket(m_timeSignal.getDescriptor(), sampleCount, m_samplesCaptured);
        m_channel.asPtr<IMtig710Channel>()->addData(domainPacket, data, sampleCount);
        m_samplesCaptured += sampleCount;
    }
    catch (const std::exception& e)
    {
        LOG_W("addData failed: {}", e.what());
        throw;
    }
}

/**
 * When the module is loaded and it implemnts a device, the device is started.
 */
void Mtig710DeviceImpl::start()
{
    configure();
}

/**
 * When the module is unloaded / openDAQ instance destroyed you may need define stop conditions. 
 */
void Mtig710DeviceImpl::stop()
{
    if (!m_started)
        return;
    m_started = false;
}

void Mtig710DeviceImpl::readProperties()
{
    m_sampleRate = objPtr.getPropertyValue("SampleRate");
    LOG_I("Properties: SampleRate {}", m_sampleRate);
}

/**
 * Do something when properties are changed. 
 */
void Mtig710DeviceImpl::propertyChanged()
{
    auto lock = getRecursiveConfigLock();

    stop();
    readProperties();
    start();
}

/**
 * A time signal set the data signal in a domain relation. 
 * Time signals are implicit signals and are calculated via a delta rule.
 * If a device has a tick resolution of 10.000 means that the device can have 
 * maximum every 10ms a value. With the delta t the steps are manipulated.
 * If the sample rate is then as well 10.000 the delta is 1.
 * If the sample rate is set to 1.000 the delta is 10.
 */
void Mtig710DeviceImpl::configureTimeSignal()
{
    auto dataDescriptor = DataDescriptorBuilder()
                              .setSampleType(SampleType::Int64)
                              .setTickResolution(Ratio(1, 10000))
                              .setRule(LinearDataRule(1, 0))
                              .setUnit(Unit("s", -1, "second", "time"))
                              .setName("Time")
                              .build();

    m_timeSignal.setDescriptor(dataDescriptor);
}

void Mtig710DeviceImpl::configure()
{
    m_channel.asPtr<IMtig710Channel>()->configure(m_timeSignal);
    configureTimeSignal();
}

/**
 * Define the conenction string of an openDAQ device. It is requried that it has 
 * this format xxx://yyy
 */
std::string Mtig710DeviceImpl::getConnectionStringFromId(std::string id)
{
    std::string connStr = "camera://cameraABC";
    return connStr;
}

/***
 * The Devie Type
 */
DeviceTypePtr Mtig710DeviceImpl::createType()
{
    return DeviceType("Mtig710", "Mtig710 device", "A camera device which es able to read cameras connected to the PC.", "camera");
}

/**
 * Help method.
 */
std::string Mtig710DeviceImpl::getIdFromConnectionString(std::string connectionString)
{
    std::string prefix = "camera://";
    auto found = connectionString.find(prefix);
    if (found != 0)
        throw InvalidParameterException();

    return connectionString.substr(prefix.size(), std::string::npos);
}

END_NAMESPACE_MTIG710_DEVICE_MODULE
