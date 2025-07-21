#pragma once

#include <iostream>
#include <string>

#include <opendaq/opendaq.h>

#include <hbk/opendaq/to_string.h>

namespace hbk::opendaq
{
    class printDescriptor
    {
        public:

            printDescriptor(
                std::ostream& os,
                const daq::DataDescriptorPtr& descriptor,
                const std::string& indent = "")
            {
                os << indent << "Name: " << to_string(descriptor.getName()) << std::endl;
                printRule(os, descriptor.getRule(), indent);
                os << indent << "Sample Type: " << to_string(descriptor.getSampleType()) << std::endl;
                os << indent << "Sample Size: " << descriptor.getSampleSize() << std::endl;
                os << indent << "Raw Sample Size: " << descriptor.getRawSampleSize() << std::endl;
                printUnit(os, descriptor.getUnit(), indent);
                printValueRange(os, descriptor.getValueRange(), indent);
                printPostScaling(os, descriptor.getPostScaling(), indent);
                if (descriptor.getOrigin().assigned())
                    os << indent << "Origin: " << to_string(descriptor.getOrigin()) << std::endl;
                if (descriptor.getTickResolution().assigned())
                    os << indent << "Tick resolution: " << to_string(descriptor.getTickResolution()) << std::endl;
                printReferenceDomainInfo(os, descriptor.getReferenceDomainInfo(), indent);
                printDimensions(os, descriptor.getDimensions(), indent);
                printStructFIelds(os, descriptor.getStructFields(), indent);
                printMetadata(os, descriptor.getMetadata(), indent);
            }

        private:

            static void printDimension(std::ostream& os, const daq::DimensionPtr& dim, const std::string& indent)
            {
                if (!dim.assigned())
                    return;

                os << indent << "Dimension: " << to_string(dim.getName()) << std::endl;
                os << indent << "    Size: " << dim.getSize() << std::endl;
                printDimensionRule(os, dim.getRule(), indent + "    ");
                if (dim.getLabels().assigned())
                    os << indent << "    Labels: (" << dim.getLabels().getCount() << ')' << std::endl;
            }

            static void printDimensions(std::ostream& os, const daq::ListPtr<daq::IDimension>& dims, const std::string& indent)
            {
                if (!dims.assigned() || dims.getCount() == 0)
                    return;

                os << indent << "Dimensions (" << dims.getCount() << "):" << std::endl;
                for (const auto& dim : dims)
                    printDimension(os, dim, indent + "    ");
            }

            static void printDimensionRule(std::ostream& os, const daq::DimensionRulePtr& rule, const std::string& indent)
            {
                if (!rule.assigned())
                    return;

                os << indent << "Rule: " << to_string(rule.getType()) << std::endl;
                printParameters(os, rule.getParameters(), indent);
            }

            static void printMetadata(std::ostream& os, const daq::DictPtr<daq::IString, daq::IString>& metadata, const std::string& indent)
            {
                if (!metadata.assigned() || metadata.getCount() == 0)
                    return;

                os << indent << "Metadata:" << std::endl;

                for (const auto& [key, value] : metadata)
                {
                    os << indent << "    " << to_string(key) << " = " << to_string(value) << std::endl;
                }
            }

            static void printParameters(std::ostream& os, const daq::DictPtr<daq::IString, daq::IBaseObject>& params, const std::string& indent)
            {
                if (!params.assigned() || params.getCount() == 0)
                    return;

                os << indent << "Parameters:" << std::endl;

                for (const auto& [key, value] : params)
                {
                    os << indent << "    " << to_string(key) << " = " << value << std::endl;
                }
            }

            static void printPostScaling(std::ostream& os, const daq::ScalingPtr& scaling, const std::string& indent)
            {
                if (!scaling.assigned())
                    return;

                os << indent << "Post-Scaling:" << std::endl;
                os << indent << "    Input Sample Type: " << to_string(scaling.getInputSampleType()) << std::endl;
                os << indent << "    Output Sample Type: " << to_string(scaling.getOutputSampleType()) << std::endl;
                printParameters(os, scaling.getParameters(), indent + "    ");
            }

            static void printReferenceDomainInfo(std::ostream& os, const daq::ReferenceDomainInfoPtr& info, const std::string& indent)
            {
                if (!info.assigned())
                    return;

                os << indent << "Reference Domain Info: " << info.getReferenceDomainId() << std::endl;
                os << indent << "    Offset: " << to_string(info.getReferenceDomainOffset()) << std::endl;
                os << indent << "    Time Source: " << to_string(info.getReferenceTimeSource()) << std::endl;
                os << indent << "    Uses Offset: " << to_string(info.getUsesOffset()) << std::endl;
            }

            static void printRule(std::ostream& os, const daq::DataRulePtr& rule, const std::string& indent)
            {
                if (!rule.assigned())
                    return;

                os << indent << "Rule: " << to_string(rule.getType()) << std::endl;
                printParameters(os, rule.getParameters(), indent + "    ");
            }

            static void printStructFIelds(std::ostream& os, const daq::ListPtr<daq::IDataDescriptor>& fields, const std::string& indent)
            {
                if (!fields.assigned() || fields.getCount() == 0)
                    return;

                os << indent << "Struct Fields (" << fields.getCount() << "):" << std::endl;
                for (const auto& field : fields)
                {
                    os << indent << "    Field: " << field.getName() << std::endl;
                    printDescriptor(os, field, indent + "        ");
                }
            }

            static void printUnit(std::ostream& os, const daq::UnitPtr& unit, const std::string& indent)
            {
                if (!unit.assigned())
                    return;

                os << indent << "Unit: " << unit.getId() << std::endl;
                os << indent << "    Name: " << to_string(unit.getName()) << std::endl;
                os << indent << "    Symbol: " << to_string(unit.getSymbol()) << std::endl;
                os << indent << "    Quantity: " << to_string(unit.getQuantity()) << std::endl;
            }

            static void printValueRange(std::ostream& os, const daq::RangePtr& range, const std::string& indent)
            {
                if (!range.assigned())
                    return;

                os << indent << "Value Range: "
                    << '[' << range.getLowValue() << ", " << range.getHighValue() << ']' << std::endl;
            }
    };
}
