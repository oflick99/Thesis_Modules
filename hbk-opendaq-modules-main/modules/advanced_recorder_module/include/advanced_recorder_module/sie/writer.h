#pragma once

#include <advanced_recorder_module/sie/basic_writer.h>
#include <advanced_recorder_module/sie/indexed_writer.h>

namespace hbk::sie
{
    /**
     * A basic_writer specialization which uses the normal indexed_writer class for the
     * implementation of the block indexing layer of SIE file writing.
     */
    typedef basic_writer<indexed_writer> writer;
}
