#pragma once

#include <cstdint>

#include <boost/endian/conversion.hpp>

#include <advanced_recorder_module/sie/xml.h>

namespace hbk::sie
{
    /**
     * A magic number that appears in the header of each block in an SIE file. The presence of the
     * synchronization word improves the ability to recognize and recover a corrupted SIE file.
     */
    static constexpr std::uint32_t SYNC_WORD = 0x51EDA7A0u;

    /**
     * Contains constants for standard group IDs defined by the SIE specification.
     */
    namespace groups
    {
        /** The group ID for XML metadata blocks. */
        static constexpr std::uint32_t METADATA = 0;

        /** The group ID for index blocks. */
        static constexpr std::uint32_t INDEX = 1;
    }

#pragma pack(push, 1)
    /**
     * The header structure that appears at the beginning of each block in an SIE file, before the
     * payload bytes. All members of this structure must be big-endian, requiring byte-swapping on
     * little-endian architectures.
     */
    struct block_header
    {
        std::uint32_t size;     /**< The size of the block, including header and footer. */
        std::uint32_t group;    /**< The group ID of the block. 0 is reserved for XML metadata,
                                     and 1 is reserved for index blocks. All other group IDs are
                                     user-defined. */
        std::uint32_t sync;     /**< Must be set to SYNC_WORD. */
    };
#pragma pack(pop)

#pragma pack(push, 1)
    /**
     * The footer structure that appears at the end of each block in an SIE file, after the
     * payload bytes. All members of this structure must be big-endian, requiring byte-swapping on
     * little-endian architectures.
     */
    struct block_footer
    {
        std::uint32_t checksum; /**< The CRC-32 checksum of the header and payload. */
        std::uint32_t size;     /**< The size of the block, including header and footer. */
    };
#pragma pack(pop)

#pragma pack(push, 1)
    /**
     * An entry for a single block in an index block. An index block consists of a sequence of
     * these structures, each of which specifies the file offset of a preceding block. All members
     * of this structure must be big-endian, requiring byte-swapping on little-endian
     * architectures.
     */
    struct index_entry
    {
        std::uint64_t offset;   /**< The absolute offset of the block in the file. */
        std::uint32_t group;    /**< The group ID of the block. */

        /**
         * Initializes an index entry with the specified member values.
         *
         * @param offset The absolute offset of the block in the file.
         * @param group The group ID of the block.
         */
        index_entry(std::uint64_t offset, std::uint32_t group) noexcept
            : offset(offset)
            , group(group)
        {
        }
    };
#pragma pack(pop)

    /**
     * The standard XML metadata preamble defined by the SIE specification. The XML metadata in
     * group 0 should always begin with this string.
     */
    constexpr const char PREAMBLE[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"                                                "\n"
        "<sie version=\"0.1\" xmlns=\"http://www.somat.com/SIE\">"                                  "\n"
        ""                                                                                          "\n"
        "<!-- SIE Format standard definitions -->"                                                  "\n"
        ""                                                                                          "\n"
        "<!-- SIE Stream decoder -->"                                                               "\n"
        ""                                                                                          "\n"
        " <decoder id=\"0\">"                                                                       "\n"
        "  <loop>"                                                                                  "\n"
        "   <read var=\"size\" bits=\"32\" type=\"uint\" endian=\"big\"/>"                          "\n"
        "   <read var=\"group\" bits=\"32\" type=\"uint\" endian=\"big\"/>"                         "\n"
        "   <read var=\"magic\" bits=\"32\" type=\"uint\" endian=\"big\" value=\"0x51EDA7A0\"/>"    "\n"
        "   <read var=\"payload\" octets=\"{$size - 20}\" type=\"raw\" group=\"{$group}\"/>"        "\n"
        "   <read var=\"checksum\" bits=\"32\" type=\"uint\" endian=\"big\"/>"                      "\n"
        "   <read var=\"size2\" bits=\"32\" type=\"uint\" endian=\"big\" value=\"{$size}\"/>"       "\n"
        "  </loop>"                                                                                 "\n"
        " </decoder>"                                                                               "\n"
        ""                                                                                          "\n"
        " <tag id=\"sie:xml_metadata\" group=\"0\" format=\"text/xml\"/>"                         "\n"
        ""                                                                                          "\n"
        "<!-- Index Block decoder: v0=offset, v1=group -->"                                         "\n"
        ""                                                                                          "\n"
        " <tag id=\"SIE:BlockIndex\" group=\"1\" decoder=\"1\"/>"                                   "\n"
        ""                                                                                          "\n"
        " <decoder id=\"1\">"                                                                       "\n"
        "  <loop>"                                                                                  "\n"
        "   <read var=\"v0\" bits=\"64\" type=\"uint\" endian=\"big\"/>"                            "\n"
        "   <read var=\"v1\" bits=\"32\" type=\"uint\" endian=\"big\"/>"                            "\n"
        "   <sample/>"                                                                              "\n"
        "  </loop>"                                                                                 "\n"
        " </decoder>"                                                                               "\n"
        ""                                                                                          "\n"
        "<!-- Stream-specific definitions begin here -->"                                           "\n"
        ""                                                                                          "\n";

    inline constexpr const char *native_endian()
    {
        return boost::endian::order::native == boost::endian::order::big ? "big" : "little";
    }
}
