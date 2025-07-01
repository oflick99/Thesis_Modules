#include <mtig710_device_module/mtig710_device_module_impl.h>
#include <mtig710_device_module/mtig710_device_impl.h>
#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>


BEGIN_NAMESPACE_MTIG710_DEVICE_MODULE

Mtig710DeviceModule::Mtig710DeviceModule(const ContextPtr& context)
    : Module("Mtig710DeviceModule",
            daq::VersionInfo(0, 0, 1),
            context,
            "Mtig710DeviceModule")
    , m_deviceIndex(0)
{
}

ListPtr<IDeviceInfo> Mtig710DeviceModule::onGetAvailableDevices()
{

    auto availableDevices = List<IDeviceInfo>();
    auto info = Mtig710DeviceImpl::CreateDeviceInfo();
    availableDevices.pushBack(info);

    return availableDevices;
}

/**
 * Is called when the openDAQ instance ask for available device types.
 */
DictPtr<IString, IDeviceType> Mtig710DeviceModule::onGetAvailableDeviceTypes()
{
    auto result = Dict<IString, IDeviceType>();

    auto deviceType = Mtig710DeviceImpl::createType();
    result.set(deviceType.getId(), deviceType);

    return result;
}

/**
 * Is called when a device is created by the module manager of the SDK. 
 * By starting openDAQ the instance loads all modules found under a specifc path. 
 */
DevicePtr Mtig710DeviceModule::onCreateDevice(const StringPtr& connectionString,
                                            const ComponentPtr& parent,
                                            const PropertyObjectPtr& /*config*/)
{
    std::scoped_lock lock(m_sync);

    std::string localId = fmt::format("Mtig710{}", m_deviceIndex++);

    auto devicePtr = createWithImplementation<IDevice, Mtig710DeviceImpl>(context, parent, StringPtr(localId));
    return devicePtr;
}

END_NAMESPACE_MTIG710_DEVICE_MODULE
