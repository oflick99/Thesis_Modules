#pragma once

#include <cstdint>

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/signal_handler.h>
#include <advanced_recorder_module/sie/writer.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

class ScalarLinearSignalHandler : public SignalHandler
{
    public:

        static bool supports(
            const SignalPtr& signal,
            const DataDescriptorPtr& valueDescriptor,
            const DataDescriptorPtr& domainDescriptor);

        ScalarLinearSignalHandler(
            hbk::sie::writer& writer,
            unsigned testId,
            const SignalPtr& signal,
            const DataDescriptorPtr& valueDescriptor,
            const DataDescriptorPtr& domainDescriptor);

        void onDataPacketReceived(const DataPacketPtr& packet) override;

    private:

        hbk::sie::writer& writer;
        std::uint32_t group;
};

END_NAMESPACE_ADVANCED_RECORDER_MODULE
