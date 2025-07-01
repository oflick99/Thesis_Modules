#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <string>
#include <system_error>
#include <utility>

namespace hbk::sie
{
    /**
     * Implements vectored (scatter/gather) file output using the C file API in a non-vectored
     * fallback mode. This implementation can be used when no vectored I/O APIs are available for
     * a target platform. This class implements the following file operations:
     *
     * - Opening: Files are opened when an object is constructed.
     * - Writing: The write() member function implements output using multiple calls to
     *   std::fwrite().
     * - Closing: Files are closed when an object is destroyed.
     */
    class fallback_vector_io_file
    {
        public:

            /**
             * Opens a file.
             *
             * @param filename The path and filename of the file to open. Relative paths are
             *     interpreted relative to the current working directory.
             *
             * @throws std::system_error The file could not be opened.
             */
            fallback_vector_io_file(const std::string& filename)
                : f(
                    std::fopen(
                        filename.c_str(),
                        "wb"))
            {
                if (!f)
                    throw std::system_error(
                        errno,
                        std::generic_category(),
                        "failed to open file");
            }

            fallback_vector_io_file(const fallback_vector_io_file&) = delete;
            fallback_vector_io_file& operator=(const fallback_vector_io_file&) = delete;

            /**
             * Moves a file. After the call, @p rhs is in an invalid state and its members should
             * not be accessed.
             *
             * @param rhs The object to move from.
             */
            fallback_vector_io_file(fallback_vector_io_file&& rhs) noexcept
                : f(nullptr)
            {
                std::swap(f, rhs.f);
            }

            /**
             * Performs a vectored write to the file.
             *
             * @param data A pointer to the first segment of data to write to the file.
             * @param size The number of bytes pointed to by @p data.
             * @param args Zero or more additional (@p data, @p size) argument pairs specifying
             *     additional segments to write to the file.
             *
             * @throws std::runtime_error A write error occurred, or an end-of-file condition
             *     occurred before completely writing all specified data.
             */
            template <typename ... Args>
            void write(const void *data, std::size_t size, Args... args)
            {
                while (size > 0)
                {
                    auto written = std::fwrite(data, 1, size, f);

                    if (std::ferror(f))
                        throw std::runtime_error(
                            "failed to write to file");

                    else if (std::feof(f) || written == 0)
                        throw std::runtime_error(
                            "failed to write to file: premature EOF");

                    else if (written >= size)
                        break;

                    size -= written;
                }

                write(args...);
            }

            /**
             * Closes a file.
             */
            ~fallback_vector_io_file() noexcept
            {
                if (f)
                    std::fclose(f);
            }

        private:

            /**
             * Terminates the recursion of the write(const void *, std::size_t, Args...) member
             * function.
             */
            static void write() noexcept
            {
            }

            /**
             * The C file handle of the file. If the file has been closed (because this object has
             * been moved-from), the value is nullptr.
             */
            FILE *f;
    };
}
