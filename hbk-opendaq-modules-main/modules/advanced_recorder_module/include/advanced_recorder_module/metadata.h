#include <cstdint>
#include <utility>

#include <opendaq/opendaq.h>

#include <advanced_recorder_module/common.h>
#include <advanced_recorder_module/sie/xml.h>


BEGIN_NAMESPACE_ADVANCED_RECORDER_MODULE

inline hbk::sie::xml::element
tickResolutionToTransform(const DataDescriptorPtr& descriptor)
{
    double scale = 1;

    auto resolution = descriptor.getTickResolution();
    if (resolution.assigned())
        scale = static_cast<double>(resolution.getNumerator())
            / static_cast<double>(resolution.getDenominator());

    return hbk::sie::transform(scale, 0);
}

inline std::pair<std::int64_t, std::int64_t>
getLinearRuleStartDelta(const DataDescriptorPtr& descriptor)
{
    auto rule = descriptor.getRule();
    if (!rule.assigned())
        throw InvalidParameterException("Descriptor is not linear-rule");

    auto params = rule.getParameters();
    if (!params.assigned())
        throw InvalidParameterException("Descriptor is missing required linear-rule parameters");

    return std::make_pair<std::int64_t, std::int64_t>(
        params.getOrDefault("start", 0),
        params.getOrDefault("delta", 1));
}

inline std::pair<const char *, unsigned>
sampleTypeToSieReadType(const DataDescriptorPtr& descriptor)
{
    switch (descriptor.getSampleType())
    {
        case SampleType::Float32:   return std::make_pair("float", 32);
        case SampleType::Float64:   return std::make_pair("float", 64);
        case SampleType::UInt8:     return std::make_pair("uint", 8);
        case SampleType::Int8:      return std::make_pair("int", 8);
        case SampleType::UInt16:    return std::make_pair("uint", 16);
        case SampleType::Int16:     return std::make_pair("int", 16);
        case SampleType::UInt32:    return std::make_pair("uint", 32);
        case SampleType::Int32:     return std::make_pair("int", 32);
        case SampleType::UInt64:    return std::make_pair("uint", 64);
        case SampleType::Int64:     return std::make_pair("int", 64);

        default:
            throw InvalidParameterException(
                "Sample type is not a scalar and has no SIE read type equivalent");
    }
}

END_NAMESPACE_ADVANCED_RECORDER_MODULE
