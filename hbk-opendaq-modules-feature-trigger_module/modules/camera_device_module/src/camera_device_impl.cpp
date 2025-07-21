//#include <boost/locale.hpp>

#include <camera_device_module/camera_device_impl.h>
#include <camera_device_module/camera_channel_impl.h>

#include <coreobjects/unit_factory.h>

#include <opendaq/device_info_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/device_domain_factory.h>

#include <opencv2/opencv.hpp>

//using namespace cv;

BEGIN_NAMESPACE_CAMERA_DEVICE_MODULE

CameraDeviceImpl::CameraDeviceImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : GenericDevice<>(ctx, parent, localId)
    , m_started(false)
    , m_logger(ctx.getLogger())
    , m_loggerComponent( m_logger.assigned()
                          ? m_logger.getOrAddComponent("CameraDeviceModule")
                          : throw ArgumentNullException("Logger must not be null"))
    , m_sampleRate(10000)
    , m_samplesCaptured(0)
    , m_quitFlag(false)
{
    // time signal is owned by device, because in case of multiple channels they should share the same time signal
    m_timeSignal = createAndAddSignal("time", nullptr, false);

    std::vector<CameraInfo> cameras = getAvailableCameras();

    initProperties();
    createCameraChannels(cameras);

    start();
    
    // 1ms cycle rate
    this->setDeviceDomain(DeviceDomain(Ratio(1, 1000),
                                       "",
                                       UnitBuilder().setName("second").setSymbol("s").setQuantity("time").build()));
}

CameraDeviceImpl::~CameraDeviceImpl()
{
    stop();
}

DeviceInfoPtr CameraDeviceImpl::CreateDeviceInfo()
{
    auto devInfo = DeviceInfo(getConnectionStringFromId(""));
    devInfo.setName("CameraDevice");
    devInfo.setModel("123");
    devInfo.setDeviceType(createType());

    return devInfo;
}

/**
 * On get info is called during the device discovery process. All information,
 * which can be defined in the device info are visible if the client calls
 * isntande.getAvailableDevices()
 */
DeviceInfoPtr CameraDeviceImpl::onGetInfo()
{
    auto deviceInfo = CreateDeviceInfo();
    deviceInfo.freeze();
    return deviceInfo;
}

uint64_t CameraDeviceImpl::onGetTicksSinceOrigin()
{
    return 0;
}

/**
 * Defines a property and sets 
 */
void CameraDeviceImpl::initProperties()
{
    auto sampleRatePropInfo = IntPropertyBuilder("SampleRate", 10).setReadOnly(true).setSuggestedValues(List<Int>(1, 10, 100)).build();
    objPtr.addProperty(sampleRatePropInfo);
    objPtr.getOnPropertyValueWrite("SampleRate") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

/**
 * Every device needs a channels, if it delivers measurement values. 
 * A channels is placed below an ioFolder.
 */
void CameraDeviceImpl::createCameraChannels(const std::vector<CameraInfo>& cameras)
{
    for (size_t i = 0; i < cameras.size(); i++)
    {
        std::string localId = "Channel" + std::to_string(i);

        VideoChannelInfo video_channel;
        video_channel.cameraIndex = cameras[i].index;
        video_channel.channel = createAndAddChannel<CameraChannelImpl>(ioFolder, localId);
        m_videoChannels.push_back(video_channel);
    }
}

// Thread collecting frames from the video camera
void CameraDeviceImpl::cameraDataThreadFunction(const std::atomic_bool& quit_flag, const VideoChannelInfo& video_channel, unsigned interval_milliseconds)
{
    const auto sleep_time = std::chrono::milliseconds(interval_milliseconds);

    // Create a video capture object and use the camera to capture the video.
    cv::VideoCapture video_capture(video_channel.cameraIndex);

    // Check if camera opened successfully.
    if (!video_capture.isOpened())
    {
        return;
    }

    //// Default resolutions of the frame are obtained. Default resolutions are system dependent.
    //int frame_width = static_cast<int>(video_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    //int frame_height = static_cast<int>(video_capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    //// Define the codec and create video writer object. The output is stored in a file.
    //cv::VideoWriter video_writer("capture.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, cv::Size(frame_width, frame_height));

    while (!quit_flag)      // keep running as long as quit flag is not set
    {
        cv::Mat frame;

        // Capture frame by frame.
        video_capture >> frame;

        // If the frame is empty, break immediately.
        if (frame.empty())
            break;

        //// Write the frame into the file.
        //video_writer.write(frame);

        std::vector<uchar> buffer;
        cv::imencode(".jpg", frame, buffer);

        addData(video_channel.channel, buffer.data(), buffer.size());

        std::this_thread::sleep_for(sleep_time);
    }

    video_capture.release();
    //video_writer.release();
}

/**
 * The data is added via callback. IF you have an SDK which ready the data from another device out, you need to define how the add Data pacakge it calles.
 * Maybe a thread is needed.
 */
void CameraDeviceImpl::addData(const ChannelPtr& channel, const uchar* data, size_t sampleCount)
{
    try
    {
        auto domainPacket = DataPacket(m_timeSignal.getDescriptor(), sampleCount, m_samplesCaptured);
        channel.asPtr<ICameraChannel>()->addData(domainPacket, data, sampleCount);
        m_samplesCaptured += sampleCount;
    }
    catch (const std::exception& e)
    {
        LOG_W("addData failed: {}", e.what())
        throw;
    }
}

/**
 * When the module is loaded, and it implements a device, the device is started.
 */
void CameraDeviceImpl::start()
{
    if (m_started)
        return;

    configure();

    // Calculate the interval time (in ms) for the camera thread from the sample rate (in Hz).
    double period = static_cast<double>(1) / m_sampleRate;
    int intervalTime = static_cast<int>(period * 1000.0);

    m_quitFlag = m_started;

    // Start a thread for each video channel.
    for (auto element : m_videoChannels)
        m_threads.push_back(std::thread(std::bind(&CameraDeviceImpl::cameraDataThreadFunction, this, std::ref(m_quitFlag), element, intervalTime)));

    m_started = true;
}

/**
 * When the module is unloaded / openDAQ instance destroyed you may need define stop conditions. 
 */
void CameraDeviceImpl::stop()
{
    //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    if (!m_started)
        return;

    m_quitFlag = m_started;

    // Wait until all threads finished.
    for (size_t i = 0; i < m_threads.size(); i++)
        m_threads[i].join();

    m_started = false;
}

void CameraDeviceImpl::readProperties()
{
    m_sampleRate = objPtr.getPropertyValue("SampleRate");
    LOG_I("Properties: SampleRate {}", m_sampleRate)
}

/**
 * Do something when properties are changed. 
 */
void CameraDeviceImpl::propertyChanged()
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
void CameraDeviceImpl::configureTimeSignal()
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

void CameraDeviceImpl::configure()
{
    for (auto element : m_videoChannels)
        element.channel.asPtr<ICameraChannel>()->configure(m_timeSignal);

    configureTimeSignal();
}

/**
 * Define the connection string of an openDAQ device. It is required that it has 
 * this format xxx://yyy
 */
std::string CameraDeviceImpl::getConnectionStringFromId(std::string id)
{
    std::string connStr = "camera://cameraABC";
    return connStr;
}

/***
 * The Device Type
 */
DeviceTypePtr CameraDeviceImpl::createType()
{
    return DeviceType("Camera", "Camera device", "A camera device which es able to read cameras connected to the PC.", "camera");
}

/**
 * Help method.
 */
std::string CameraDeviceImpl::getIdFromConnectionString(std::string connectionString)
{
    std::string prefix = "camera://";
    auto found = connectionString.find(prefix);
    if (found != 0)
        throw InvalidParameterException();

    return connectionString.substr(prefix.size(), std::string::npos);
}

std::vector<CameraDeviceImpl::CameraInfo> CameraDeviceImpl::getAvailableCameras()
{
    std::vector<CameraInfo> cameras;

    for (int index = 0; index < 100; index++)
    {
        cv::VideoCapture video_capture(index);

        if (video_capture.isOpened())
        {
            CameraInfo info;
            info.index = index;
            info.description = video_capture.getBackendName();
            cameras.push_back(info);
        }
        video_capture.release();
    }
    return cameras;
}

END_NAMESPACE_CAMERA_DEVICE_MODULE
