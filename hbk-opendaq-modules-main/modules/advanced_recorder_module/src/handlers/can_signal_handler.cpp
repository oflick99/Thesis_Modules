#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/metadata.h>
#include <advanced_recorder_module/handlers/can_signal_handler.h>
#include <advanced_recorder_module/sie/writer.h>
#include <advanced_recorder_module/sie/xml.h>

#pragma pack(push, 1)
struct opendaq_can_message
{
    std::uint32_t id;
    std::uint8_t size;
    std::uint8_t data[64];
};
struct sie_can_message
{
    std::int64_t domain;
    std::uint8_t size;
    std::uint32_t id;
    std::uint8_t data[64];
};
#pragma pack(pop)

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

bool CanSignalHandler::supports(
    const SignalPtr& signal,
    const DataDescriptorPtr& valueDescriptor,
    const DataDescriptorPtr& domainDescriptor)
{
    // The domain must be explicit-rule.
    auto domainRule = domainDescriptor.getRule();
    if (!domainRule.assigned() || domainRule.getType() != DataRuleType::Explicit)
    {
        std::cout << "[advanced-recorder] rejecting signal because domain is not explicit-rule" << std::endl;
        return false;
    }

    // The value must be explicit-rule.
    auto valueRule = valueDescriptor.getRule();
    if (!valueRule.assigned() || valueRule.getType() != DataRuleType::Explicit)
    {
        std::cout << "[advanced-recorder] rejecting signal because value is not explicit-rule" << std::endl;
        return false;
    }

    // The value must be a struct type.
    auto type = valueDescriptor.getSampleType();
    if (type != SampleType::Struct)
    {
        std::cout << "[advanced-recorder] rejecting signal because value type is not struct" << std::endl;
        return false;
    }

    // XXX TODO

    return true;
}

CanSignalHandler::CanSignalHandler(
        hbk::sie::writer& writer,
        unsigned testId,
        const SignalPtr& signal,
        const DataDescriptorPtr& valueDescriptor,
        const DataDescriptorPtr& domainDescriptor)
    : writer(writer)
    , group(writer.allocate_group())
{
    unsigned decoderId = writer.allocate_decoder();
    unsigned channelId = writer.allocate_channel();

    auto dim0 = hbk::sie::dimension(0)
        .add_child(tickResolutionToTransform(domainDescriptor))
        .add_child(hbk::sie::data(decoderId, 0));

    if (auto unit = domainDescriptor.getUnit(); unit.assigned())
        dim0.add_child(hbk::sie::units(unit.getName()));
    auto channel = hbk::sie::channel(channelId, group, valueDescriptor.getName())
        .add_child(hbk::sie::tag("core:description", "Raw CAN messages"))
        .add_child(hbk::sie::tag("data_type", "message_can"))
        .add_child(hbk::sie::tag("somat:datamode_type", "message_log"))
        .add_child(hbk::sie::tag("core:schema", "somat:message"))
        .add_child(std::move(dim0));

    auto [type, bits] = sampleTypeToSieReadType(domainDescriptor);
    unsigned bytes = bits / 8;

    auto loop = hbk::sie::xml::element("loop")
        .add_attribute("var", "i")
        .add_attribute("start", "0")
        .add_attribute("end", "{$n}")
        .add_child(hbk::sie::read("v0", type, bits))
        .add_child(hbk::sie::seek("start", "{" + std::to_string(sizeof(std::uint32_t)) + " + (" + std::to_string(bytes) + " * $n) + (69 * $i)}"));

    unsigned dimIndex = 1;
    for (const auto& field : valueDescriptor.getStructFields())
    {
        auto dim = hbk::sie::dimension(dimIndex)
            .add_child(hbk::sie::tag("openDAQ:fieldName", field.getName()))
            .add_child(hbk::sie::data(decoderId, dimIndex));

        if ((field.getSampleType() == SampleType::Int8 || field.getSampleType() == SampleType::UInt8)
                && field.getDimensions().assigned()
                && field.getDimensions().getCount() == 1
                && field.getDimensions().getItemAt(0).getRule().getType() == DimensionRuleType::Linear)
        {
            std::size_t octets = field.getDimensions().getItemAt(0).getSize();
            loop.add_child(hbk::sie::read_raw("v" + std::to_string(dimIndex), octets));
        }

        else
        {
            auto [fieldType, size] = sampleTypeToSieReadType(field);
            loop.add_child(hbk::sie::read("v" + std::to_string(dimIndex), fieldType, size));
        }

        channel.add_child(std::move(dim));

        ++dimIndex;
    }

    loop
        .add_child(hbk::sie::sample())
        .add_child(hbk::sie::seek("start", "{" + std::to_string(sizeof(std::uint32_t) + bytes) + " + (" + std::to_string(bytes) + " * $i)}"));

    auto test = hbk::sie::test(testId)
        .add_child(std::move(channel));

    auto decoder = hbk::sie::decoder(decoderId)
        .add_child(hbk::sie::read("n", "uint", 32))
        .add_child(std::move(loop));

    std::ostringstream os;
    decoder.serialize(os, 1);
    test.serialize(os, 1);

    writer.write_metadata(os.str());
}

void CanSignalHandler::onDataPacketReceived(const DataPacketPtr& packet)
{
    // We can't currently handle packets without a domain packet.
    auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
        return;

    // The way CAN messages are formatted in openDAQ is incompatible with expectations that
    // existing applications place on the CAN message structure. It is not possible to craft an
    // SIE decoder that transforms the representations. Specifically, the CAN message ID and
    // payload must be adjacent, and there is no sample dimension output concatenation capability
    // in the decoder schema. Therefore we must preprocess the data.

    if (packet.getSampleCount() > std::numeric_limits<std::size_t>::max())
        return;
    std::uint32_t N = static_cast<std::uint32_t>(packet.getSampleCount());

    if (packet.getRawDataSize() < N * sizeof(opendaq_can_message))
        return;
    if (domainPacket.getRawDataSize() < N * sizeof(std::int64_t))
        return;

    writer.write_block(group,
        &N,                         sizeof(N),
        domainPacket.getRawData(),  domainPacket.getRawDataSize(),
        packet.getRawData(),        packet.getRawDataSize());
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
