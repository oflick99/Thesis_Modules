#pragma once

#include <advanced_recorder_module/sie/basic_block_writer.h>
#include <advanced_recorder_module/sie/vector_io_file.h>

namespace hbk::sie
{
    /**
     * A basic_block_writer specialization which uses the normal vector_io_file class for vectored
     * (scatter/gather) I/O.
     */
    typedef basic_block_writer<vector_io_file> block_writer;
}
