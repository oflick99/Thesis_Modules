#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

#include <advanced_recorder_module/sie/format.h>

namespace hbk::sie
{
    /**
     * Implements the logical layer of SIE file writing functionality. This class operates on an
     * indexed_writer or block_writer and keeps track of the unique channel, decoder, group and
     * test identifiers that have been used so far in the file. It provides the
     * allocate_channel(), allocate_decoder(), allocate_group() and allocate_test() functions for
     * this purpose. It also adds the write_metadata() function for writing XML strings to the
     * special metadata group (group 0).
     *
     * This class is implemented using template-based dependency injection. This pattern allows
     * for better reuse and unit-testing.
     *
     * @tparam Writer A type which implements the block or block indexing layer of SIE file
     *     writing. This type must be noexcept-moveable and must provide a write_block() function.
     */
    template <typename Writer>
    class basic_writer
    {
        public:

            /**
             * Creates a new writer.
             *
             * @param writer An @p Writer object which is moved-into the constructed object. After
             *     the call, @p writer is in an invalid state and its members should not be
             *     accessed.
             */
            basic_writer(Writer&& writer) noexcept
                : writer(std::move(writer))
            {
            }

            /**
             * Allocate and returns a unique channel identifier.
             *
             * @return A channel identifier which has not yet been used in this SIE file.
             */
            unsigned allocate_channel() noexcept
            {
                return next_channel_id++;
            }

            /**
             * Allocate and returns a unique decoder identifier.
             *
             * @return A decoder identifier which has not yet been used in this SIE file.
             */
            unsigned allocate_decoder() noexcept
            {
                return next_decoder_id++;
            }

            /**
             * Allocate and returns a unique group number.
             *
             * @return A group number which has not yet been used in this SIE file and which is
             *     not reserved for a special purpose by the SIE specification.
             */
            std::uint32_t allocate_group() noexcept
            {
                return next_group++;
            }

            /**
             * Allocate and returns a unique test identifier.
             *
             * @return A test identifier which has not yet been used in this SIE file.
             */
            unsigned allocate_test() noexcept
            {
                return next_test_id++;
            }

            /**
             * Writes an XML string to the SIE file's special metadata group (group 0).
             *
             * @param xml The XML string to write.
             *
             * @throws ... This function propagates any exception thrown by Writer::write_block().
             */
            void write_metadata(const std::string& xml)
            {
                write_block(
                    hbk::sie::groups::METADATA,
                    xml.data(),
                    xml.size());
            }

            /**
             * @copydoc basic_block_writer::write_block()
             */
            template <typename ... Args>
            void write_block(
                std::uint32_t group,
                Args&&... args)
            {
                writer.write_block(group, args...);
            }

        private:

            std::atomic<unsigned> next_channel_id = 0;
            std::atomic<unsigned> next_decoder_id = 2;
            std::atomic<std::uint32_t> next_group = 2;
            std::atomic<unsigned> next_test_id = 2;

            Writer writer;
    };
}
