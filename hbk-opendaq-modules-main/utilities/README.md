# HBK openDAQ Utilities

This directory contains a header-only utility library for HBK code that uses openDAQ. The library
includes convenience classes and functions that simplify certain aspects of working with and
debugging openDAQ user code. At the moment, the library is header-only, although compiled code may
be added in the future.

# Using the Library

The top-level CMakeLists.txt already includes the library via the following command:

    add_subdirectory(utilities)

To link a module to the library, add the following:

    target_link_libraries(
        my_module
        PRIVATE
            hbk::opendaq_utils)

Then include the desired headers; for example:

    #include <iostream>
    #include <hbk/opendaq/print_descriptor.h>

    hbk::opendaq::print_descriptor(std::cout, myDataDescriptor);
