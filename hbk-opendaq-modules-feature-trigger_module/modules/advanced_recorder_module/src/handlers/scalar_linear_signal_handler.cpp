#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/metadata.h>
#include <advanced_recorder_module/handlers/scalar_linear_signal_handler.h>
#include <advanced_recorder_module/sie/writer.h>
#include <advanced_recorder_module/sie/xml.h>

BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

static const char *openDaqSampleTypeToSieDataType(SampleType type)
{
    switch (type)
    {
        case SampleType::Float32: return "sequential_float32";
        case SampleType::Float64: return "sequential_float64";
        case SampleType::UInt8: return "sequential_uint8";
        case SampleType::Int8: return "sequential_int8";
        case SampleType::UInt16: return "sequential_uint16";
        case SampleType::Int16: return "sequential_int16";
        case SampleType::UInt32: return "sequential_uint32";
        case SampleType::Int32: return "sequential_int32";
        case SampleType::UInt64: return "sequential_uint64";
        case SampleType::Int64: return "sequential_int64";

        default:
            throw InvalidParameterException(
                "Unsupported openDAQ sample type " + std::to_string(static_cast<int>(type)));
    }
}

static const char *openDaqSampleTypeToSieDataFormat(SampleType type)
{
    switch (type)
    {
        case SampleType::Float32: return "float";
        case SampleType::Float64: return "float";
        case SampleType::UInt8: return "uint";
        case SampleType::UInt16: return "uint";
        case SampleType::UInt32: return "uint";
        case SampleType::UInt64: return "uint";
        case SampleType::Int8: return "int";
        case SampleType::Int16: return "int";
        case SampleType::Int32: return "int";
        case SampleType::Int64: return "int";

        default:
            throw InvalidParameterException(
                "Unsupported openDAQ sample type " + std::to_string(static_cast<int>(type)));
    }
}

static unsigned openDaqSampleTypeToSieBits(SampleType type)
{
    switch (type)
    {
        case SampleType::Float32: return 32;
        case SampleType::Float64: return 64;
        case SampleType::UInt8: return 8;
        case SampleType::Int8: return 8;
        case SampleType::UInt16: return 16;
        case SampleType::Int16: return 16;
        case SampleType::UInt32: return 32;
        case SampleType::Int32: return 32;
        case SampleType::UInt64: return 64;
        case SampleType::Int64: return 64;

        default:
            throw InvalidParameterException(
                "Unsupported openDAQ sample type " + std::to_string(static_cast<int>(type)));
    }
}

bool ScalarLinearSignalHandler::supports(
    const SignalPtr& signal,
    const DataDescriptorPtr& valueDescriptor,
    const DataDescriptorPtr& domainDescriptor)
{
    // The domain must be linear-rule.
    auto domainRule = domainDescriptor.getRule();
    if (!domainRule.assigned() || domainRule.getType() != DataRuleType::Linear)
        return false;

    // The value must be explicit-rule.
    auto valueRule = valueDescriptor.getRule();
    if (!valueRule.assigned() || valueRule.getType() != DataRuleType::Explicit)
        return false;

    // The value must be a scalar type.
    auto type = valueDescriptor.getSampleType();
    if (type != SampleType::Float32 &&
        type != SampleType::Float64 &&
        type != SampleType::UInt8 &&
        type != SampleType::Int8 &&
        type != SampleType::UInt16 &&
        type != SampleType::Int16 &&
        type != SampleType::UInt32 &&
        type != SampleType::Int32 &&
        type != SampleType::UInt64 &&
        type != SampleType::Int64)
        return false;

    // The value must not have custom dimensions.
    auto dimensions = valueDescriptor.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() > 0)
        return false;

    return true;
}

ScalarLinearSignalHandler::ScalarLinearSignalHandler(
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

    auto [start, delta] = getLinearRuleStartDelta(domainDescriptor);
    auto [type, bits] = sampleTypeToSieReadType(valueDescriptor);

    double resolution = 1;
    if (auto tickResolution = domainDescriptor.getTickResolution(); tickResolution.assigned())
        resolution = static_cast<double>(tickResolution.getNumerator())
            / static_cast<double>(tickResolution.getDenominator());
    double sampleRate = 1.0 / resolution / delta;

    auto decoder = hbk::sie::decoder(decoderId)
        .add_child(hbk::sie::read("offset", "int", 8 * sizeof(std::int64_t)))
        .add_child(
            hbk::sie::xml::element("loop")
                .add_attribute("var", "v0")
                .add_attribute("start", "{$offset + " + std::to_string(start) + "}")
                .add_attribute("increment", std::to_string(delta))
                .add_child(hbk::sie::read("v1", type, bits))
                .add_child(hbk::sie::sample())
        );

    auto dim0 = hbk::sie::dimension(0)
        .add_child(tickResolutionToTransform(domainDescriptor))
        .add_child(hbk::sie::data(decoderId, 0));

    if (auto unit = domainDescriptor.getUnit(); unit.assigned())
        dim0.add_child(hbk::sie::units(unit.getName()));

    auto dim1 = hbk::sie::dimension(1)
        .add_child(hbk::sie::data(decoderId, 1));

    if (auto unit = valueDescriptor.getUnit(); unit.assigned())
        dim1.add_child(hbk::sie::units(unit.getName()));

    if (auto range = valueDescriptor.getValueRange(); range.assigned())
    {
        double min = range.getLowValue();
        double max = range.getHighValue();

        dim1.add_child(hbk::sie::tag("FS_Min", std::to_string(min)));
        dim1.add_child(hbk::sie::tag("FS_Max", std::to_string(max)));
    }

    auto channel = hbk::sie::channel(channelId, group, valueDescriptor.getName())
        .add_child(hbk::sie::tag("core:uuid", boost::uuids::to_string(boost::uuids::random_generator()())))
        .add_child(hbk::sie::tag("data_type", openDaqSampleTypeToSieDataType(valueDescriptor.getSampleType())))
        .add_child(hbk::sie::tag("somat:data_format", openDaqSampleTypeToSieDataFormat(valueDescriptor.getSampleType())))
        .add_child(hbk::sie::tag("core:description", signal.getDescription()))
        .add_child(hbk::sie::tag("somat:input_channel", signal.getGlobalId()))
        .add_child(hbk::sie::tag("core:sample_rate", std::to_string(sampleRate)))
        .add_child(hbk::sie::tag("somat:data_bits", std::to_string(openDaqSampleTypeToSieBits(valueDescriptor.getSampleType()))))
        .add_child(hbk::sie::tag("core:schema", "somat:sequential"))
        .add_child(std::move(dim0))
        .add_child(std::move(dim1));

    auto test = hbk::sie::test(testId)
        .add_child(std::move(channel));

    std::ostringstream os;
    decoder.serialize(os, 1);
    test.serialize(os, 1);

    writer.write_metadata(os.str());
}

void ScalarLinearSignalHandler::onDataPacketReceived(const DataPacketPtr& packet)
{
    // We can't currently handle packets without a domain packet.
    auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
        return;

    // Get the 64-bit domain value. We can't currently handle packets without a domain offset.
    auto offset = domainPacket.getOffset();
    if (!offset.assigned())
        return;
    std::int64_t domainValue = offset;

    // The data block, in accordance with the SIE decoder generated at construction, consists of
    // the 64-bit domain value followed by the raw value data.
    writer.write_block(group,
        &domainValue,           sizeof(domainValue),
        packet.getRawData(),    packet.getRawDataSize());
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
