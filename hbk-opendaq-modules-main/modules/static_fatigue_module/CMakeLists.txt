hbk_module()

if(TARGET Boost::atomic)
    list(APPEND BOOST_DEPENDENCIES Boost::atomic)
endif()
target_link_libraries(${MODULE_NAME}
    PRIVATE
        hbk::opendaq_utils
        ${BOOST_DEPENDENCIES}
    PUBLIC
        daq::opendaq
        "${CMAKE_BINARY_DIR}/bin/Debug/opendaq-64-3-debug.lib"
        "${CMAKE_BINARY_DIR}/bin/Debug/daqcoreobjects-64-3-debug.lib"
        "${CMAKE_BINARY_DIR}/bin/Debug/daqcoretypes-64-3-debug.lib"
)            