# Our tests require daq::test_utils, but this target is not installed by openDAQ. Therefore it is
# only possible to use it when building openDAQ from source (via FetchContent / add_subdirectory).
# So we will not attempt to use a preinstalled openDAQ version if testing is enabled.
if(NOT HBK_OPENDAQ_ENABLE_TESTS)
    find_package(openDAQ GLOBAL QUIET)
endif()

if(openDAQ_FOUND)

    message(STATUS "Found openDAQ ${openDAQ_VERSION} at ${openDAQ_CONFIG}")

    # SDK bug workaround: The macros in openDAQUtils.cmake rely on some variables
    # which are set in its main CMakeLists.txt files, but which are not in the
    # *Config.cmake file used by find_package().
    set(OPENDAQ_MODULE_SUFFIX ".module${CMAKE_SHARED_LIBRARY_SUFFIX}")

    # Another SDK bug workaround: The OPENDAQ_MODULES_DIR variable is unusable
    # because it's interpreted in openDAQConfig.cmake relative to the CMake file
    # itself. When cross-compiling this is wrong.
    set(OPENDAQ_MODULES_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/modules")

else()

    message(STATUS "Fetching openDAQ...")

    function(FetchOpenDAQ)

        set(OPENDAQ_ENABLE_TESTS                OFF)
        set(DAQMODULES_REF_FB_MODULE            OFF)
        set(OPENDAQ_ENABLE_OPCUA                OFF)
        set(DAQMODULES_EMPTY_MODULE             OFF)
        set(DAQMODULES_OPENDAQ_CLIENT_MODULE    OFF)
        set(DAQMODULES_OPENDAQ_SERVER_MODULE    OFF)
        set(DAQMODULES_REF_DEVICE_MODULE        ON)
        set(DAQMODULES_AUDIO_DEVICE_MODULE      OFF)
        set(OPENDAQ_ENABLE_WEBSOCKET_STREAMING  OFF)
        set(APP_ENABLE_WEBPAGE_EXAMPLES         OFF)
        set(OPENDAQ_DOCUMENTATION_TESTS         OFF)
        set(OPENDAQ_ENABLE_NATIVE_STREAMING     OFF)
        set(OPENDAQ_ALWAYS_FETCH_BOOST          OFF)

        # Against best practices, openDAQ uses ${Boost_INCLUDE_DIRS} in several places. We have
        # to feed it something to avoid CMake errors when the variable is set to NOTFOUND.
        set(Boost_INCLUDE_DIRS "")

        # Don't install openDAQ if HBK_OPENDAQ_INSTALL_OPENDAQ is not set.
        set(FETCH_PARAMS "")
        if(NOT HBK_OPENDAQ_INSTALL_OPENDAQ)
            set(FETCH_PARAMS "EXCLUDE_FROM_ALL")
        endif()

        FetchContent_Declare(openDAQ
            GIT_REPOSITORY  https://github.com/openDAQ/openDAQ.git
            GIT_TAG         v3.19.3-rc
            OVERRIDE_FIND_PACKAGE
            ${FETCH_PARAMS}
        )

        FetchContent_MakeAvailable(openDAQ)

    endfunction()

    FetchOpenDAQ()

endif()
