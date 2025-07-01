#pragma once

#include <memory>

#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/signal_handler.h>
#include <advanced_recorder_module/sie/writer.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

/*!
 * Records a single openDAQ signal in an SIE file. Objects are constructed with a reference to the
 * openDAQ signal object, and a reference to the SIE writer object used to write data blocks to
 * the file. Then the caller calls onPacketReceived() (ideally from a worker thread) to
 * synchronously record packet data.
 */
class AdvancedRecorderSignal
{
    public:

        /*!
         * Creates a signal write handler for the specified signal.
         *
         * @param signal The openDAQ signal object to be recorded.
         * @param writer A reference to the SIE writer object to write to.
         * @param testId The id of the <test> element in the SIE file.
         */
        AdvancedRecorderSignal(
            const SignalPtr& signal,
            std::shared_ptr<hbk::sie::writer> writer,
            unsigned testId);

        /*!
         * @brief Records the values in a packet to the SIE file.
         *
         * @param packet The packet received.
         *
         * @throws std::system_error Data could not be written to SIE file due to an I/O error.
         */
        void onPacketReceived(const PacketPtr& packet);

    private:

        /*!
         * @brief Records the values the specified packet to the SIE file.
         *
         * @todo Explain operation in more detail.
         *
         * @param packet The data packet to record.
         *
         * @throws std::system_error Data could not be written to SIE file due to an I/O error.
         */
        void onDataPacketReceived(const DataPacketPtr packet);

        SignalPtr signal;

        /**
         * A reference to the SIE writer object to write to.
         */
        std::shared_ptr<hbk::sie::writer> writer;

        unsigned group = 2;
        unsigned testId;

        DataDescriptorPtr lastValueDescriptor;
        DataDescriptorPtr lastDomainDescriptor;

        std::unique_ptr<SignalHandler> handler;
};

END_NAMESPACE_ADVANCED_RECORDER_MODULE
