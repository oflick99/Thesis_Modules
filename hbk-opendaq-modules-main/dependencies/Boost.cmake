find_package(Boost 1.82 QUIET GLOBAL)

if(Boost_FOUND)

    message(STATUS "Found Boost ${Boost_VERSION} at ${Boost_CONFIG}")

else()

    message(STATUS "Fetching Boost...")

    # openDAQ does naughty things to manipulate Boost CMake variables. Even though we find/fetch
    # Boost BEFORE openDAQ, subsequent CMake runs after the first one will use the manipulated
    # (polluted) variables, resulting in Boost not building the components we need. We must reset
    # the variables here so that the correct values are used on every run.
    set(BOOST_INCLUDE_LIBRARIES
        algorithm
        align
        asio
        beast
        crc
        dll
        endian
        locale
        program_options
        uuid
    )

    FetchContent_Declare(Boost
        URL             https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
        OVERRIDE_FIND_PACKAGE
    )

    FetchContent_MakeAvailable(Boost)

endif()
