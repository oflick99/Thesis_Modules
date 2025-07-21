find_package(OpenCV GLOBAL QUIET)

if(OpenCV_FOUND)

    message(STATUS "Found OpenCV ${OpenCV_VERSION} at ${OpenCV_CONFIG}")

else()

    message(STATUS "Fetching OpenCV...")
    message(STATUS "Fetch content does not work? (TOOD)")

    function(FetchOpenCV)

        set(BUILD_opencv_world  ON)
        set(BUILD_SHARED_LIBS   OFF)
        set(ENABLE_PIC          ON)
        set(BUILD_TESTS         OFF)
        set(BUILD_PERF_TESTS    OFF)
        set(BUILD_EXAMPLES      OFF)
        set(BUILD_opencs_app    OFF)

        FetchContent_Declare(OpenCV
            GIT_REPOSITORY  https://github.com/opencv/opencv.git
            GIT_TAG         4.10.0
            OVERRIDE_FIND_PACKAGE
        )

        FetchContent_MakeAvailable(OpenCV)

    endfunction()

    FetchOpenCV()

endif()
