#include <camera_device_module/camera_device_module_impl.h>
#include <camera_device_module/camera_device_impl.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>


BEGIN_NAMESPACE_CAMERA_DEVICE_MODULE

CameraDeviceModule::CameraDeviceModule(const ContextPtr& context)
    : Module("CameraDeviceModule",
            daq::VersionInfo(0, 0, 1),
            context,
            "CameraDeviceModule")
    , m_deviceIndex(0)
{
}

ListPtr<IDeviceInfo> CameraDeviceModule::onGetAvailableDevices()
{

    auto availableDevices = List<IDeviceInfo>();
    auto info = CameraDeviceImpl::CreateDeviceInfo();
    availableDevices.pushBack(info);

    return availableDevices;
}

/**
 * Is called when the openDAQ instance ask for available device types.
 */
DictPtr<IString, IDeviceType> CameraDeviceModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = CameraDeviceImpl::createType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

/**
 * Is called when a device is created by the module manager of the SDK. 
 * By starting openDAQ the instance loads all modules found under a specifc path. 
 */
DevicePtr CameraDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                            const ComponentPtr& parent,
                                            const PropertyObjectPtr& /*config*/)
{
    std::scoped_lock lock(m_sync);

    std::string localId = fmt::format("Camera{}", m_deviceIndex++);

    auto devicePtr = createWithImplementation<IDevice, CameraDeviceImpl>(context, parent, StringPtr(localId));
    return devicePtr;
}

END_NAMESPACE_CAMERA_DEVICE_MODULE
