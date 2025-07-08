#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

#include <trigger_fb_module/version.h>
#include <trigger_fb_module/trigger_fb_module_impl.h>


#include <trigger_fb_module/logic_fb_impl.h>
#include <trigger_fb_module/trigger_fb_impl.h>
#include <trigger_fb_module/trigger_gate_fb_impl.h>



BEGIN_NAMESPACE_TRIGGER_FB_MODULE

TriggerFbModule::TriggerFbModule(ContextPtr ctx)
    : Module("TriggerFunctionBlockModule",
             daq::VersionInfo(TRIGGER_FB_MODULE_MAJOR_VERSION, TRIGGER_FB_MODULE_MINOR_VERSION, TRIGGER_FB_MODULE_PATCH_VERSION),
             std::move(ctx),
             "TriggerFunctionBlockModule")
{
}

DictPtr<IString, IFunctionBlockType> TriggerFbModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto triggerGate = TriggerGateFbImpl::CreateType();
    types.set(triggerGate.getId(), triggerGate);

    const auto logic = LogicFbImpl::CreateType();
    types.set(logic.getId(), logic);

    const auto trigger = TriggerFbImpl::CreateType();
    types.set(trigger.getId(), trigger);

    return types;
}

FunctionBlockPtr TriggerFbModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
    if (id == TriggerGateFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, TriggerGateFbImpl>(context, parent, localId, config);
        return fb;
    }
    if (id == LogicFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, LogicFbImpl>(context, parent, localId, config);
        return fb;
    }
    if (id == TriggerFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, TriggerFbImpl>(context, parent, localId, config);
        return fb;
    }
    
    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

END_NAMESPACE_TRIGGER_FB_MODULE