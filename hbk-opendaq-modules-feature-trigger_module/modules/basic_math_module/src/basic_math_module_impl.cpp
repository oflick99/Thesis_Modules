#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>

#include <basic_math_module/version.h>
#include <basic_math_module/basic_math_module_impl.h>


#include <basic_math_module/math_sum_fb.h>



BEGIN_NAMESPACE_BASIC_MATH_MODULE

BasicMathModule::BasicMathModule(ContextPtr ctx)
    : Module("BasicMathFunctionBlockModule",
             daq::VersionInfo(BASIC_MATH_MODULE_MAJOR_VERSION, BASIC_MATH_MODULE_MINOR_VERSION, BASIC_MATH_MODULE_PATCH_VERSION),
             std::move(ctx),
             "BasicMathFunctionBlockModule")
{
}

DictPtr<IString, IFunctionBlockType> BasicMathModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto MathSumFb = MathSumFbImpl::CreateType();
    types.set(MathSumFb.getId(), MathSumFb);

    return types;
}

FunctionBlockPtr BasicMathModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
    if (id == MathSumFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, MathSumFbImpl>(context, parent, localId, config);
        return fb;
    }
    
    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

END_NAMESPACE_BASIC_MATH_MODULE