#pragma once

#include <map>
#include <memory>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <advanced_recorder_module/advanced_recorder_signal.h>
#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/sie/writer.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

/*!
 * @brief An advanced recorder function block which records data from its input signals into an
 *     SIE file.
 *
 * A single SIE file is created for each recorded signal. The path and filename of this file is
 * specified by the `Filename` property. Signals can be dynamically connected and disconnected.
 * Initially, the function block has a single input port named "Value1"; additional input ports
 * "Value2" etc. are created so that at least one unconnected input port is always available.
 * Signals can be connected or disconnected while the recording is active. Doing so does not
 * disturb ongoing recording of other signals.
 *
 * Packet handlers which write to the filesystem are invoked in a background thread to avoid
 * blocking the acquisition thread.
 */
class AdvancedRecorderImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
    public:

        /*!
         * @brief The type ID of this function block.
         */
        static constexpr const char *TYPE_ID = "AdvancedRecorder";

        /*!
         * @brief Contains constants for the names of tags assigned to this function block.
         */
        struct Tags
        {
            /*!
             * @brief A tag identifying this function block as a recorder.
             */
            static constexpr const char *RECORDER = "Recorder";
        };

        /*!
         * Contains constants for the names of properties supported by this function block.
         */
        struct Props
        {
            /*!
             * @brief The path and filename of the SIE file to write.
             *
             * The current implementation interprets relative paths with respect to the current
             * working directory of the process, but this behavior is not guaranteed.
             */
            static constexpr const char *FILENAME = "Filename";
        };

        /*!
         * @brief Creates and returns a type object describing this function block.
         * @returns A populated function block type object.
         */
        static FunctionBlockTypePtr createType();

        /*!
         * @brief Creates a new function block.
         * @param context The openDAQ context object.
         * @param parent The component object which will contain this function block.
         * @param localId The local identifier of this function block.
         * @param config A property object containing configuration data for this function block.
         */
        AdvancedRecorderImpl(
            const ContextPtr& context,
            const ComponentPtr& parent,
            const StringPtr& localId,
            const PropertyObjectPtr& config);

        /*!
         * @brief Starts the recording, if it was not already started.
         * @return OPENDAQ_SUCCESS.
         */
        ErrCode INTERFACE_FUNC startRecording() override;

        /*!
         * @brief Stops the recording, if it was started.
         * @return OPENDAQ_SUCCESS.
         */
        ErrCode INTERFACE_FUNC stopRecording() override;

        /*!
         * @brief Checks whether data from connected signals is currently being recorded to the
         *     persistent storage medium.
         * @param isRecording A pointer to a boolean which is populated with the recording state.
         * @retval OPENDAQ_SUCCESS if the recording state was successfully stored.
         * @retval OPENDAQ_ERR_ARGUMENT_NULL if @p isRecording is `nullptr`.
         */
        ErrCode INTERFACE_FUNC getIsRecording(Bool *isRecording) override;

        /*!
         * @brief When a signal is connected to an input port, a new input port is dynamically
         *     added so that one is always available to be connected.
         * @param port The input port that was connected.
         */
        void onConnected(const InputPortPtr& port) override;

        /*!
         * @brief When a signal is disconnected from the second-to-last input port, the last input
         *     port is removed, so that only one unconnected port is always present at the end (it
         *     is however possible for additional ports to be disconnected if there are still
         *     higher-numbered ports in use).
         * @param port The input port that was disconnected.
         */
        void onDisconnected(const InputPortPtr& port) override;

        /*!
         * @brief Records a packet.
         *
         * @param port The input port on which the packet was received.
         */
        void onPacketReceived(const InputPortPtr& port) override;

    protected:

        /*!
         * @brief Stops recording when the component is deactivated.
         */
        virtual void activeChanged() override;

    private:

        void addProperties();
        void addInputPort();
        void reconfigure();

        bool recordingActive = false;

        std::shared_ptr<hbk::sie::writer> writer;

        std::shared_ptr<AdvancedRecorderSignal> findSignal(IInputPort *port);

        std::map<IInputPort *, std::shared_ptr<AdvancedRecorderSignal>> signals;

        unsigned portCount = 0;
};

END_NAMESPACE_ADVANCED_RECORDER_MODULE
