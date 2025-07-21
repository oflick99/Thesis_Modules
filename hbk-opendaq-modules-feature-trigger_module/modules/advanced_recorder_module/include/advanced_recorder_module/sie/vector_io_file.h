#pragma once

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <advanced_recorder_module/sie/posix_vector_io_file.h>

namespace hbk::sie
{
    /**
     * A typedef which refers to the best available VectorIoFile implementation detected for the
     * current platform (e.g., posix_vector_io_file or fallback_vector_io_file).
     */
    typedef posix_vector_io_file vector_io_file;
}

#else

#include <advanced_recorder_module/sie/fallback_vector_io_file.h>

namespace hbk::sie
{
    /**
     * A typedef which refers to the best available VectorIoFile implementation detected for the
     * current platform (e.g., posix_vector_io_file or fallback_vector_io_file).
     */
    typedef fallback_vector_io_file vector_io_file;
}

#endif
