#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

namespace hbk::sie::xml
{
    /**
     * Represents an XML attribute.
     */
    struct attribute
    {
        std::string id;     /**< The identifier (name) of the attribute. */
        std::string value;  /**< The value of the attribute. */

        /**
         * Initializes an attribute with the specified identifier (name) and value.
         *
         * @param id The identifier (name) of the attribute.
         * @param value The value of the attribute.
         */
        attribute(const std::string& id, const std::string& value)
            : id(id)
            , value(value)
        {
        }
    };

    /**
     * Represents an XML element. This structure exposes a "fluent API" which can be used to
     * construct trees of XML by chaining member function calls. XML elements can be serialized to
     * an output stream by calling serialize().
     */
    struct element
    {
        std::string name_;                  /**< The name (tag) of the element. */
        std::list<attribute> attributes_;   /**< The attributes assigned to the element. */
        std::list<element> children_;       /**< The children of the element (nested elements). */
        std::string content_;               /**< The text content of the element. */

        /**
         * Initializes a new empty element.
         *
         * @param name The name (tag) of the element.
         */
        element(const std::string& name)
            : name_(name)
        {
        }

        /**
         * Gets the name (tag) of this element.
         *
         * @return The name (tag) of this element.
         */
        const std::string& name() const noexcept
        {
            return name_;
        }

        /**
         * Gets a list of this element's attributes.
         *
         * @return A reference to a list of this element's attributes. If there are no attributes,
         *     the list is empty.
         */
        const std::list<attribute>& attributes() const noexcept
        {
            return attributes_;
        }

        /**
         * Adds an attribute to this element.
         *
         * @param id The identifier (name) of the attribute to add.
         * @param value The value of the attribute to add.
         *
         * @return An rvalue reference to this object, enabling "fluent" style chaining.
         */
        element&& add_attribute(const std::string& id, const std::string& value)
        {
            attributes_.emplace_back(id, value);
            return std::move(*this);
        }

        /**
         * Gets a list of this element's children.
         *
         * @return A reference to a list of this element's children. If there are no children, the
         *     list is empty.
         */
        const std::list<element>& children() const noexcept
        {
            return children_;
        }

        /**
         * Adds a child element to this element.
         *
         * @param name The name (tag) of the child element to add.
         *
         * @return An rvalue reference to this object, enabling "fluent" style chaining.
         */
        element&& add_child(const std::string& name)
        {
            children_.emplace_back(name);
            return std::move(*this);
        }

        /**
         * Adds a child element to this element.
         *
         * @param element The child element to add.
         *
         * @return An rvalue reference to this object, enabling "fluent" style chaining.
         */
        element&& add_child(element&& element)
        {
            children_.emplace_back(std::move(element));
            return std::move(*this);
        }

        /**
         * Gets the text content of this element.
         *
         * @return The text content of this element.
         */
        const std::string& content() const noexcept
        {
            return content_;
        }

        /**
         * Sets the text content of this element.
         *
         * @param content The text content of this element.
         *
         * @return An rvalue reference to this object, enabling "fluent" style chaining.
         */
        element&& content(const std::string& content)
        {
            content_ = content;
            return std::move(*this);
        }

        /**
         * Recursively serializes this XML element, and its attributes and children, to the
         * specified output stream.
         *
         * @param os The output stream to write to.
         * @param indent The initial indentation level. Nested elements are automatically indented
         *     by one space per level.
         *
         * @return @p os.
         */
        std::ostream& serialize(
            std::ostream& os,
            unsigned indent = 0
        ) const;
    };
}

namespace hbk::sie
{
    xml::element channel(unsigned id, std::uint32_t group, const std::string& name);

    xml::element data(unsigned decoderId, unsigned variableId);

    xml::element decoder(unsigned id);

    xml::element dimension(unsigned id);

    xml::element read(const std::string& var, const std::string& type, unsigned bits);

    xml::element read_raw(const std::string& var, std::size_t octets);

    xml::element read_raw(const std::string& var, const std::string& octets);

    xml::element sample();

    xml::element seek(const char *from, const std::string& offset);

    xml::element set(const char *var, const std::string& value);

    xml::element tag(const char *id, const std::string& value);

    xml::element test(unsigned id);

    xml::element transform(double scale, double offset);

    xml::element units(const std::string& name);
}
