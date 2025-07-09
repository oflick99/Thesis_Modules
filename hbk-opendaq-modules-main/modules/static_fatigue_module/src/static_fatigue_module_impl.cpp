#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

#include <static_fatigue_module/version.h>
#include <static_fatigue_module/static_fatigue_module_impl.h>


#include <static_fatigue_module/sg_subtraction_fb.h>



BEGIN_NAMESPACE_STATIC_FATIGUE_MODULE

StaticFatigueModule::StaticFatigueModule(ContextPtr ctx)
    : Module("StaticFatigueFunctionBlockModule",
             daq::VersionInfo(STATIC_FATIGUE_MODULE_MAJOR_VERSION, STATIC_FATIGUE_MODULE_MINOR_VERSION, STATIC_FATIGUE_MODULE_PATCH_VERSION),
             std::move(ctx),
             "StaticFatigueFunctionBlockModule")
{
}

DictPtr<IString, IFunctionBlockType> StaticFatigueModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto SgSubFb = SgSubtractionFbImpl::CreateType();
    types.set(SgSubFb.getId(), SgSubFb);

    return types;
}

FunctionBlockPtr StaticFatigueModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
    if (id == SgSubtractionFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, SgSubtractionFbImpl>(context, parent, localId, config);
        return fb;
    }
    
    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

END_NAMESPACE_STATIC_FATIGUE_MODULE