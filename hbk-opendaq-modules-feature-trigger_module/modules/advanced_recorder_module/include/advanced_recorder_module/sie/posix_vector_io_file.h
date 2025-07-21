#pragma once

#include <array>
#include <cerrno>
#include <cstddef>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>

namespace hbk::sie
{
    /**
     * Implements vectored (scatter/gather) file output using the POSIX API. This class implements
     * the following file operations:
     *
     * - Opening: Files are opened when an object is constructed.
     * - Writing: The write() member function implements vectored output using writev().
     * - Closing: Files are closed when an object is destroyed.
     */
    class posix_vector_io_file
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
            posix_vector_io_file(const std::string& filename)
                : fd(
                    ::open(
                        filename.c_str(),
                        O_CREAT | O_TRUNC | O_WRONLY,
                        0644))
            {
                if (fd == -1)
                    throw std::system_error(
                        errno,
                        std::generic_category(),
                        "failed to open file");
            }

            posix_vector_io_file(const posix_vector_io_file&) = delete;
            posix_vector_io_file& operator=(const posix_vector_io_file&) = delete;

            /**
             * Moves a file. After the call, @p rhs is in an invalid state and its members should
             * not be accessed.
             *
             * @param rhs The object to move from.
             */
            posix_vector_io_file(posix_vector_io_file&& rhs) noexcept
                : fd(-1)
            {
                std::swap(fd, rhs.fd);
            }

            /**
             * Performs a vectored write to the file.
             *
             * @param data A pointer to the first segment of data to write to the file.
             * @param size The number of bytes pointed to by @p data.
             * @param args Zero or more additional (@p data, @p size) argument pairs specifying
             *     additional segments to write to the file.
             *
             * @throws std::system_error A write error occurred.
             * @throws std::runtime_error An end-of-file condition occurred before completely
             *     writing all specified data.
             */
            template <typename ... Args>
            void write(const void *data, std::size_t size, Args... args)
            {
                std::array<iovec, 1 + sizeof...(args) / 2> segments;
                populate_iovs(segments.data(), data, size, args...);

                iovec *current_segment = segments.data();
                std::size_t segments_remaining = segments.size();
                std::size_t total_written = 0;

                while (segments_remaining)
                {
                    auto written = ::writev(fd, current_segment, segments_remaining);

                    if (written < 0)
                        throw std::system_error(
                            errno,
                            std::generic_category(),
                            "failed to write to file");

                    else if (written == 0)
                        throw std::runtime_error(
                            "failed to write to file: premature EOF");

                    do
                    {
                        if (static_cast<std::size_t>(written) >= current_segment->iov_len)
                        {
                            written -= static_cast<ssize_t>(current_segment->iov_len);
                            total_written += current_segment->iov_len;
                            ++current_segment;
                            --segments_remaining;
                        }

                        else
                        {
                            current_segment->iov_base = static_cast<std::uint8_t *>(current_segment->iov_base) + written;
                            current_segment->iov_len -= written;
                            total_written += written;
                            written = 0;
                        }
                    } while (written);
                }
            }

            /**
             * Closes a file.
             */
            ~posix_vector_io_file() noexcept
            {
                if (fd != -1)
                    ::close(fd);
            }

        private:

            /**
             * Terminates the recursion of the
             * populate_iovs(iovec *, const void *, std::size_t, Args...) member function.
             */
            static void populate_iovs(iovec *) noexcept
            {
            }

            /**
             * Recursively populates an array of `iovec` structures to describe the data segments
             * indicated by the pairs of @p data and @p size arguments.
             *
             * @param data A pointer to the first segment of data to write to the file.
             * @param size The number of bytes pointed to by @p data.
             * @param args Zero or more additional (@p data, @p size) argument pairs specifying
             *     additional segments to write to the file.
             */
            template <typename ... Args>
            static void populate_iovs(
                iovec *segments,
                const void *data,
                std::size_t size,
                Args... args) noexcept
            {
                segments->iov_base = const_cast<void *>(data);
                segments->iov_len = size;
                populate_iovs(segments + 1, args...);
            }

            /**
             * Terminates the recursion of the write(const void *, std::size_t, Args...) member
             * function.
             */
            static void write() noexcept
            {
            }

            /**
             * The POSIX file descriptor of the file. If the file has been closed (because this
             * object has been moved-from), the value is -1.
             */
            int fd;
    };
}
