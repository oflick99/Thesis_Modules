#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

#include <advanced_recorder_module/advanced_recorder_impl.h>
#include <advanced_recorder_module/advanced_recorder_module_impl.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

AdvancedRecorderModule::AdvancedRecorderModule(ContextPtr context)
    : Module(
        MODULE_NAME,
        daq::VersionInfo(
            ADVANCED_RECORDER_MODULE_MAJOR_VERSION,
            ADVANCED_RECORDER_MODULE_MINOR_VERSION,
            ADVANCED_RECORDER_MODULE_PATCH_VERSION),
        std::move(context),
        MODULE_NAME)
    , advancedRecorderType(AdvancedRecorderImpl::createType())
{
}

DictPtr<IString, IFunctionBlockType>
AdvancedRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto id = advancedRecorderType.getId();
    return Dict<IString, IFunctionBlockType>(
    {
        { advancedRecorderType.getId(), advancedRecorderType },
    });
}

FunctionBlockPtr AdvancedRecorderModule::onCreateFunctionBlock(
    const StringPtr& id,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config)
{
    if (id == advancedRecorderType.getId())
        return createWithImplementation<IFunctionBlock, AdvancedRecorderImpl>(context, parent, localId, config);

    throw NotFoundException("This module does not support function block type '" + id + "'");
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
