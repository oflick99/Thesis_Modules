#pragma once

#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

#include <advanced_recorder_module/sie/format.h>

namespace hbk::sie
{
    /**
     * Implements the block indexing layer of SIE file writing functionality. This class operates
     * on a block_writer and keeps track of the offset of each block written to it. It then
     * periodically emits index blocks containing the group number and offset of each
     * previously-written block.
     *
     * This class wraps the write_block() function of the underlying block layer. It also adds a
     * flush_index() function which can be called to explicitly generate an index block. Index
     * blocks are also periodically emitted automatically.
     *
     * This class is implemented using template-based dependency injection. This pattern allows
     * for better reuse and unit-testing.
     *
     * @tparam BlockWriter A type which implements the block layer of SIE file writing. This type
     *     must be noexcept-moveable and must provide a write_block() function.
     */
    template <typename BlockWriter>
    class basic_indexed_writer
    {
        public:

            /**
             * Creates a new indexed writer.
             *
             * @param writer A @p BlockWriter object which is moved-into the constructed object.
             *     After the call, @p writer is in an invalid state and its members should not be
             *     accessed.
             */
            basic_indexed_writer(BlockWriter&& writer) noexcept
                : writer(std::move(writer))
            {
            }

            /**
             * Moves an indexed writer. After the call, the referenced object is in an invalid
             * state and its members should not be accessed.
             */
            basic_indexed_writer(basic_indexed_writer&&) = default;

            /**
             * @copydoc basic_block_writer::write_block()
             *
             * This call may also autonomously emit an index block. Index blocks can also be
             * emitted manually by calling flush_index().
             */
            template <typename ... Args>
            void write_block(
                std::uint32_t group,
                Args&&... args)
            {
                if (group != groups::INDEX)
                    index.emplace_back(
                        boost::endian::native_to_big<std::uint64_t>(offset),
                        boost::endian::native_to_big<std::uint32_t>(group));

                offset += writer.write_block(group, args...);

                if (index.size() >= INDEX_EVERY)
                    flush_index();
            }

            /**
             * Explicitly emits an index block, if any not-yet-indexed blocks have been written.
             * It is normally not necessary to call this function, because index blocks are also
             * emitted autonomously by write_block().
             *
             * @throws ... This function propagates any exception thrown by
             *     BlockWriter::write_block().
             */
            void flush_index()
            {
                if (index.empty())
                    return;

                offset += writer.write_block(
                    groups::INDEX,
                    index.data(),
                    index.size() * sizeof(index_entry));

                index.clear();
            }

            /**
             * Emits a closing index block, if any not-yet-indexed blocks have been written. If an
             * I/O error occurrs, it is silently ignored.
             */
            ~basic_indexed_writer() noexcept
            {
                try
                {
                    flush_index();
                }

                catch (const std::exception&)
                {
                }
            }

        private:

            /**
             * An index block is automatically emitted after this number of non-index blocks have
             * been written via write_block(). Index blocks can also be explicitly emitted by
             * calling flush_index().
             */
            static constexpr std::size_t INDEX_EVERY = 100;

            BlockWriter writer;

            std::vector<index_entry> index;
            std::uint64_t offset = 0;
    };
}
