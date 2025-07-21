#pragma once

#include <string>

#include <opendaq/opendaq.h>

namespace hbk::opendaq
{
    template <typename Ptr>
    std::string to_string(const Ptr& ptr);

    template <>
    std::string to_string<daq::ComplexNumberPtr>(const daq::ComplexNumberPtr& complex)
    {
        if (!complex.assigned())
            return "(unassigned)";

        return std::to_string(complex.getReal())
            + " + "
            + std::to_string(complex.getImaginary()) + "i";
    }

    inline std::string to_string(daq::CoreType type)
    {
        switch (type)
        {
            case daq::CoreType::ctBool: return "ctBool";
            case daq::CoreType::ctInt: return "ctInt";
            case daq::CoreType::ctFloat: return "ctFloat";
            case daq::CoreType::ctString: return "ctString";
            case daq::CoreType::ctList: return "ctList";
            case daq::CoreType::ctDict: return "ctDict";
            case daq::CoreType::ctRatio: return "ctRatio";
            case daq::CoreType::ctProc: return "ctProc";
            case daq::CoreType::ctObject: return "ctObject";
            case daq::CoreType::ctBinaryData: return "ctBinaryData";
            case daq::CoreType::ctFunc: return "ctFunc";
            case daq::CoreType::ctComplexNumber: return "ctComplexNumber";
            case daq::CoreType::ctStruct: return "ctStruct";
            case daq::CoreType::ctEnumeration: return "ctEnumeration";
            case daq::CoreType::ctUndefined: return "ctUndefined";
            default: return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    template <>
    std::string to_string<daq::IntegerPtr>(const daq::IntegerPtr& i)
    {
        if (!i.assigned())
            return "(unassigned)";

        return std::to_string(static_cast<long>(i));
    }

    template <>
    std::string to_string<daq::RatioPtr>(const daq::RatioPtr& ratio)
    {
        if (!ratio.assigned())
            return "(unassigned)";

        return std::to_string(ratio.getNumerator())
            + " / "
            + std::to_string(ratio.getDenominator());
    }

    inline std::string to_string(daq::DataRuleType type)
    {
        switch (type)
        {
            case daq::DataRuleType::Other: return "Other";
            case daq::DataRuleType::Linear: return "Linear";
            case daq::DataRuleType::Constant: return "Constant";
            case daq::DataRuleType::Explicit: return "Explicit";
            default: return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    inline std::string to_string(daq::DimensionRuleType type)
    {
        switch (type)
        {
            case daq::DimensionRuleType::Other: return "Other";
            case daq::DimensionRuleType::Linear: return "Linear";
            case daq::DimensionRuleType::Logarithmic: return "Logarithmic";
            case daq::DimensionRuleType::List: return "List";
            default: return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    inline std::string to_string(daq::SampleType type)
    {
        switch (type)
        {
            case daq::SampleType::Invalid: return "Invalid/Undefined";
            case daq::SampleType::Float32: return "Float32";
            case daq::SampleType::Float64: return "Float64";
            case daq::SampleType::UInt8: return "UInt8";
            case daq::SampleType::Int8: return "Int8";
            case daq::SampleType::UInt16: return "UInt16";
            case daq::SampleType::Int16: return "Int16";
            case daq::SampleType::UInt32: return "UInt32";
            case daq::SampleType::Int32: return "Int32";
            case daq::SampleType::UInt64: return "UInt64";
            case daq::SampleType::Int64: return "Int64";
            case daq::SampleType::RangeInt64: return "RangeInt64";
            case daq::SampleType::ComplexFloat32: return "ComplexFloat32";
            case daq::SampleType::ComplexFloat64: return "ComplexFloat64";
            case daq::SampleType::Binary: return "Binary";
            case daq::SampleType::String: return "String";
            case daq::SampleType::Struct: return "Struct";
            default: return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    inline std::string to_string(daq::ScaledSampleType type)
    {
        switch (type)
        {
            case daq::ScaledSampleType::Invalid: return "Invalid";
            case daq::ScaledSampleType::Float32: return "Float32";
            case daq::ScaledSampleType::Float64: return "Float64";
            default: return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    template <>
    std::string to_string<daq::StringPtr>(const daq::StringPtr& str)
    {
        if (!str.assigned())
            return "(unassigned)";

        return str.getCharPtr();
    }

    inline std::string to_string(daq::TimeSource source)
    {
        switch (source)
        {
            case daq::TimeSource::Unknown: return "Unknown";
            case daq::TimeSource::Tai: return "Tai";
            case daq::TimeSource::Gps: return "Gps";
            case daq::TimeSource::Utc: return "Utc";
            default: return "Unknown (" + std::to_string(static_cast<int>(source)) + ")";
        }
    }

    inline std::string to_string(daq::UsesOffset uses)
    {
        switch (uses)
        {
            case daq::UsesOffset::Unknown: return "Unknown";
            case daq::UsesOffset::True: return "True";
            case daq::UsesOffset::False: return "False";
            default: return "Unknown (" + std::to_string(static_cast<int>(uses)) + ")";
        }
    }

    template <>
    std::string to_string<daq::BaseObjectPtr>(const daq::BaseObjectPtr& obj)
    {
        if (!obj.assigned())
            return "(unassigned)";

        switch (obj.getCoreType())
        {
            case daq::CoreType::ctBool: return static_cast<bool>(obj) ? "True" : "False";
            case daq::CoreType::ctInt: return std::to_string(static_cast<int>(obj));
            case daq::CoreType::ctFloat: return std::to_string(static_cast<double>(obj));
            case daq::CoreType::ctString: return static_cast<std::string>(obj);
            case daq::CoreType::ctList: return "IList [" + std::to_string(obj.asPtr<daq::IList>().getCount()) + "]";
            case daq::CoreType::ctDict: return "IDict [" + std::to_string(obj.asPtr<daq::IDict>().getCount()) + "]";
            case daq::CoreType::ctRatio: return to_string(obj.asPtr<daq::IRatio>());
            case daq::CoreType::ctComplexNumber: return to_string(obj.asPtr<daq::IComplexNumber>());
            default:
                return to_string(obj.getCoreType());
        }
    }
}
