cmake_minimum_required(VERSION 3.24)

project(hbk-opendaq-modules VERSION 0.0.1 LANGUAGES C CXX)

# Project options.
option(HBK_OPENDAQ_ENABLE_ADVANCED_RECORDER "Build the advanced recorder module"    ON)
option(HBK_OPENDAQ_ENABLE_CAMERA_DEVICE     "Build the camera device module"        OFF)
option(HBK_OPENDAQ_ENABLE_MTIG710_DEVICE    "Build the MTi-G-710 device module"     ON)
option(HBK_OPENDAQ_ENABLE_PLAYBACK_DEVICE   "Build the playback device module"      ON)
option(HBK_OPENDAQ_ENABLE_APPS              "Build demo/sample/test apps"           ON)
option(HBK_OPENDAQ_ENABLE_TESTS             "Build unit tests"                      ON)
option(HBK_OPENDAQ_INSTALL_OPENDAQ          "Install openDAQ (if it was fetched)"   OFF)

include(FetchContent)
include(cmake/hbk_module.cmake)
include(dependencies/Boost.cmake)
include(dependencies/openDAQ.cmake)
include(dependencies/Threads.cmake)

if(HBK_OPENDAQ_ENABLE_TESTS)
    enable_testing()
endif()

add_subdirectory(utilities)

if(HBK_OPENDAQ_ENABLE_ADVANCED_RECORDER)
    add_subdirectory("modules/advanced_recorder_module")
endif()

if(HBK_OPENDAQ_ENABLE_CAMERA_DEVICE)
    include(dependencies/OpenCV.cmake)
    add_subdirectory("modules/camera_device_module")
endif()

if(HBK_OPENDAQ_ENABLE_MTIG710_DEVICE)
    add_subdirectory("modules/mtig710_device_module")
endif()

if(HBK_OPENDAQ_ENABLE_PLAYBACK_DEVICE)
    add_subdirectory("modules/playback_device_module")
endif()

if(HBK_OPENDAQ_ENABLE_APPS)
    add_subdirectory(apps)
endif()
