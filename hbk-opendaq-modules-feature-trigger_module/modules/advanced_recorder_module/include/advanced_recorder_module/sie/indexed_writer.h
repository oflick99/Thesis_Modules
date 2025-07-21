#pragma once

#include <advanced_recorder_module/sie/basic_indexed_writer.h>
#include <advanced_recorder_module/sie/block_writer.h>

namespace hbk::sie
{
    /**
     * A basic_indexed_writer specialization which uses the normal block_writer class for
     * the implementation of the block layer of SIE file writing.
     */
    typedef basic_indexed_writer<block_writer> indexed_writer;
}
