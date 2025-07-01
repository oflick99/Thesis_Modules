#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

#include <boost/endian/conversion.hpp>
#include <boost/crc.hpp>

#include <advanced_recorder_module/sie/format.h>

namespace hbk::sie
{
    /**
     * Implements the block layer of SIE file writing. This is the lowest level of the SIE file
     * format, in which an SIE file is written as a consecutive sequence of blocks. Each block
     * consists of a payload, sandwiched with a header and footer, which specify a group number,
     * the payload size, and a checksum.
     *
     * This class provides the write_block() function, which supports vectored (scatter/gather)
     * writes by accepting one or more pairs of arguments describing the payload data segments.
     *
     * This class is implemented using template-based dependency injection. This pattern allows
     * for better reuse and unit-testing.
     *
     * @tparam VectorIoFile A type which implements vectored (scatter/gather) file output. This
     *     type must be noexcept-moveable and must provide a write() function which accepts one or
     *     more pairs of data/size arguments describing the data segments to write.
     */
    template <typename VectorIoFile>
    class basic_block_writer
    {
        public:

            /**
             * Creates a new block writer.
             *
             * @param file A @p VectorIoFile object which is moved-into the constructed object.
             *     After the call, @p file is in an invalid state and its members should not be
             *     accessed.
             */
            basic_block_writer(VectorIoFile&& file) noexcept
                : file(std::move(file))
            {
            }

            /**
             * Moves a block writer. After the call, the referenced object is in an invalid state
             * and its members should not be accessed.
             */
            basic_block_writer(basic_block_writer&&) noexcept = default;

            /**
             * Writes a block to the SIE file.
             *
             * @param group The group ID of the block. See the hbk::sie::groups namespace for
             *     constants for group IDs defined by the SIE specification.
             * @param args One or more pairs of arguments describing the payload data. Each
             *     argument pair must consist of a pointer to the data to write, and a std::size_t
             *     specifying the number of bytes pointed to. The written payload is the
             *     concatenation of the data described by each argument pair.
             *
             * @throws ... This function propagates any exception thrown by VectorIoFile::write().
             */
            template <typename ... Args>
            std::size_t write_block(
                std::uint32_t group,
                Args&&... args)
            {
                block_header header;
                block_footer footer;

                std::size_t payload_size = get_payload_size(args...);

                header.size = boost::endian::native_to_big<std::uint32_t>(
                    sizeof(header) + payload_size + sizeof(footer));
                header.group = boost::endian::native_to_big<std::uint32_t>(group);
                header.sync = boost::endian::native_to_big<std::uint32_t>(SYNC_WORD);

                boost::crc_32_type crc;
                crc.process_bytes(&header, sizeof(header));
                do_payload_crc(crc, args...);

                footer.checksum = boost::endian::native_to_big<std::uint32_t>(crc());
                footer.size = header.size;

                file.write(
                    &header, sizeof(header),
                    args...,
                    &footer, sizeof(footer));

                return sizeof(header) + payload_size + sizeof(footer);
            }

        private:

            static std::size_t get_payload_size()
            {
                return 0;
            }

            template <typename ... Args>
            static std::size_t get_payload_size(const void *data, std::size_t size, Args... args)
            {
                return size + get_payload_size(args...);
            }

            static void do_payload_crc(boost::crc_32_type& crc)
            {
            }

            template <typename ... Args>
            static void do_payload_crc(boost::crc_32_type& crc,
                const void *data, std::size_t size, Args... args)
            {
                crc.process_bytes(data, size);
                do_payload_crc(crc, args...);
            }

            VectorIoFile file;
    };
}
