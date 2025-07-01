#include <exception>
#include <iostream>
#include <memory>
#include <utility>

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/advanced_recorder_signal.h>
#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/handlers/can_signal_handler.h>
#include <advanced_recorder_module/handlers/scalar_linear_signal_handler.h>
#include <advanced_recorder_module/sie/writer.h>

#include <hbk/opendaq/print_descriptor.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

AdvancedRecorderSignal::AdvancedRecorderSignal(
        const SignalPtr& signal,
        std::shared_ptr<hbk::sie::writer> writer,
        unsigned testId)
    : signal(signal)
    , writer(std::move(writer))
    , testId(testId)
{
}

void AdvancedRecorderSignal::onPacketReceived(const PacketPtr& packet)
{
    switch (packet.getType())
    {
        case PacketType::Data:
            onDataPacketReceived(packet);
            break;

        default:
            break;
    }
}

void AdvancedRecorderSignal::onDataPacketReceived(DataPacketPtr packet)
{
    // If the descriptors have changed (or we never saw a descriptor),
    // close out the channel and start a new one.
    auto valueDescriptor = packet.getDataDescriptor();
    auto domainPacket = packet.getDomainPacket();
    auto domainDescriptor = domainPacket.assigned() ? domainPacket.getDataDescriptor() : nullptr;

    if (valueDescriptor != lastValueDescriptor || domainDescriptor != lastDomainDescriptor)
    {
        handler.reset();

        lastValueDescriptor = valueDescriptor;
        lastDomainDescriptor = domainDescriptor;

        try
        {
            if (ScalarLinearSignalHandler::supports(signal, valueDescriptor, domainDescriptor))
                handler = std::make_unique<ScalarLinearSignalHandler>(
                    *writer,
                    testId,
                    signal,
                    valueDescriptor,
                    domainDescriptor);

            else if (CanSignalHandler::supports(signal, valueDescriptor, domainDescriptor))
                handler = std::make_unique<CanSignalHandler>(
                    *writer,
                    testId,
                    signal,
                    valueDescriptor,
                    domainDescriptor);

            else
            {
                std::cerr << "[advanced-recorder] No suitable SIE signal handler found for '" << signal.getGlobalId() << "'." << std::endl;
                std::cerr << "[advanced-recorder] Its value descriptor is:" << std::endl;
                hbk::opendaq::printDescriptor(std::cerr, valueDescriptor, "    ");
                std::cerr << "[advanced-recorder] Its domain descriptor is:" << std::endl;
                hbk::opendaq::printDescriptor(std::cerr, domainDescriptor, "    ");
            }
        }

        catch (const std::exception& ex)
        {
            // XXX TODO
            std::cerr << "[advanced-recorder] failed to create handler for signal: " << ex.what() << std::endl;
            handler.reset();
        }
    }

    if (handler)
        handler->onDataPacketReceived(packet);
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
