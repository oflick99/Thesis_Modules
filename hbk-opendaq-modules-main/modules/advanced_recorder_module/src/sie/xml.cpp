#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

#include <advanced_recorder_module/sie/format.h>
#include <advanced_recorder_module/sie/xml.h>

struct escaped_string
{
    const std::string& str;

    escaped_string(const std::string& str) noexcept
        : str(str)
    {
    }
};

std::ostream& operator<<(std::ostream& os, const escaped_string& str)
{
    for (char ch : str.str)
    {
        switch (ch)
        {
            case '"':
                os << "&quot;";
                break;

            case '\'':
                os << "&apos;";
                break;

            case '<':
                os << "&lt;";
                break;

            case '>':
                os << "&gt;";
                break;

            case '&':
                os << "&amp;";
                break;

            default:
                os << ch;
                break;
        }
    }

    return os;
}

std::ostream&
hbk::sie::xml::element::serialize(
    std::ostream& os,
    unsigned indent
) const
{
    os
        << std::string(indent, ' ')
        << '<'
        << name();

    for (const auto& attribute : attributes())
        os
            << ' '
            << attribute.id
            << "=\""
            << escaped_string(attribute.value)
            << '"';

    if (content().empty() && children().empty())
    {
        os << "/>\n";
    }

    else
    {
        if (!content().empty())
        {
            os
                << '>'
                << escaped_string(content());
            os
                << "</"
                << name()
                << ">\n";
        }

        else
        {
            os << ">\n";
            for (const auto child : children())
                child.serialize(os, indent + 1);
            os << std::string(indent, ' ');
            os
                << "</"
                << name()
                << ">\n";
        }
    }

    return os;
}

hbk::sie::xml::element hbk::sie::channel(unsigned id, std::uint32_t group, const std::string& name)
{
    return xml::element("ch")
        .add_attribute("id", std::to_string(id))
        .add_attribute("group", std::to_string(group))
        .add_attribute("name", name);
}

hbk::sie::xml::element hbk::sie::data(unsigned decoderId, unsigned variableId)
{
    return xml::element("data")
        .add_attribute("decoder", std::to_string(decoderId))
        .add_attribute("v", std::to_string(variableId));
}

hbk::sie::xml::element hbk::sie::decoder(unsigned id)
{
    return xml::element("decoder")
        .add_attribute("id", std::to_string(id));
}

hbk::sie::xml::element hbk::sie::dimension(unsigned id)
{
    return xml::element("dim")
        .add_attribute("index", std::to_string(id));
}

hbk::sie::xml::element hbk::sie::read(const std::string& var, const std::string& type, unsigned bits)
{
    return xml::element("read")
        .add_attribute("var", var)
        .add_attribute("type", type)
        .add_attribute("bits", std::to_string(bits))
        .add_attribute("endian", hbk::sie::native_endian());
}

hbk::sie::xml::element hbk::sie::read_raw(const std::string& var, std::size_t octets)
{
    return xml::element("read")
        .add_attribute("var", var)
        .add_attribute("type", "raw")
        .add_attribute("octets", std::to_string(octets));
}

hbk::sie::xml::element hbk::sie::read_raw(const std::string& var, const std::string& octets)
{
    return xml::element("read")
        .add_attribute("var", var)
        .add_attribute("type", "raw")
        .add_attribute("octets", octets);
}

hbk::sie::xml::element hbk::sie::sample()
{
    return xml::element("sample");
}

hbk::sie::xml::element hbk::sie::seek(const char *from, const std::string& offset)
{
    return xml::element("seek")
        .add_attribute("from", from)
        .add_attribute("offset", offset);
}

hbk::sie::xml::element hbk::sie::set(const char *var, const std::string& value)
{
    return xml::element("set")
        .add_attribute("var", var)
        .add_attribute("value", value);
}

hbk::sie::xml::element hbk::sie::tag(const char *id, const std::string& value)
{
    return xml::element("tag")
        .add_attribute("id", id)
        .content(value);
}

hbk::sie::xml::element hbk::sie::test(unsigned id)
{
    return xml::element("test")
        .add_attribute("id", std::to_string(id));
}

hbk::sie::xml::element hbk::sie::transform(double scale, double offset)
{
    std::ostringstream scaleStr;
    scaleStr << std::scientific << scale;

    std::ostringstream offsetStr;
    offsetStr << std::scientific << offset;

    return xml::element("xform")
        .add_attribute("scale", scaleStr.str())
        .add_attribute("offset", offsetStr.str());
}

hbk::sie::xml::element hbk::sie::units(const std::string& name)
{
    return hbk::sie::xml::element("units")
        .content(name);
}
