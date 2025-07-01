#pragma once

#include <opendaq/module_impl.h>

#include <advanced_recorder_module/common.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

/*!
 * @brief An openDAQ module which exposes a single `Recorder` object, AdvancedRecorderImpl.
 */
class AdvancedRecorderModule final : public Module
{
    public:

        /*!
         * @brief The name of this module.
         */
        static constexpr const char *MODULE_NAME = "AdvancedRecorderModule";

        /*!
         * @brief Constructs a new module.
         * @param context A reference to the openDAQ context object.
         */
        AdvancedRecorderModule(ContextPtr context);

        /*!
         * @brief Returns a dictionary of the function block types supported by this module.
         * @returns A dictionary of the function block types supported by this module.
         */
        DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;

        /*!
         * @brief Creates and returns a new function block.
         * @param id The identifier of the type of function block to create.
         * @param parent The component object which will contain the function block.
         * @param localId The local identifier of the function block.
         * @param config A property object containing configuration data for the function block.
         * @return A smart pointer to the created function block.
         * @throws NotFoundException This module does not support the specified function block
         *     type.
         */
        FunctionBlockPtr onCreateFunctionBlock(
            const StringPtr& id,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const PropertyObjectPtr& config) override;

    private:

        FunctionBlockTypePtr advancedRecorderType;
};

END_NAMESPACE_ADVANCED_RECORDER_MODULE
