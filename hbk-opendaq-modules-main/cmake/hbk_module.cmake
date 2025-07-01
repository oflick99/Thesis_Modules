function(hbk_module)

    set_cmake_folder_context(TARGET_FOLDER_NAME)

    get_filename_component(MODULE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

    set(MODULE_NAME "${MODULE_NAME}" PARENT_SCOPE)
    set(MODULE_NAME_UPPER "${MODULE_NAME_UPPER}" PARENT_SCOPE)

    file(GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
                              "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

    add_library(${MODULE_NAME} SHARED ${sources} ${headers})
    add_library(daq::${MODULE_NAME} ALIAS ${MODULE_NAME})

    set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD                17
        CXX_STANDARD_REQUIRED       ON
        CXX_EXTENSIONS              ON
        ARCHIVE_OUTPUT_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY   "${CMAKE_BINARY_DIR}/bin"
    )

    if (MSVC)
        target_compile_options(${MODULE_NAME} PRIVATE /bigobj)
    endif()

    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            ${MODULE_NAME_UPPER}_MAJOR_VERSION=${PROJECT_VERSION_MAJOR}u
            ${MODULE_NAME_UPPER}_MINOR_VERSION=${PROJECT_VERSION_MINOR}u
            ${MODULE_NAME_UPPER}_PATCH_VERSION=${PROJECT_VERSION_PATCH}u
    )

    target_link_libraries(
        ${MODULE_NAME}
        PUBLIC
            daq::opendaq
        PRIVATE
            hbk::opendaq_utils
    )

    target_include_directories(
        ${MODULE_NAME}
        PUBLIC
            include
            "${CMAKE_CURRENT_BINARY_DIR}/include"
    )

    opendaq_set_module_properties(${MODULE_NAME} ${PROJECT_VERSION_MAJOR})

    opendaq_create_version_header(
        ${MODULE_NAME}
        "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}"
        ""
        OFF
        ON
    )

    create_version_header(${MODULE_NAME})

    if(HBK_OPENDAQ_ENABLE_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")

        file(GLOB_RECURSE sources tests/*.cpp)
        file(GLOB_RECURSE headers tests/*.h tests/*.hpp)

        add_executable(test_${MODULE_NAME} ${sources})

        set_target_properties(test_${MODULE_NAME} PROPERTIES
            CXX_STANDARD                17
            CXX_STANDARD_REQUIRED       ON
            CXX_EXTENSIONS              ON
            RUNTIME_OUTPUT_DIRECTORY    "${CMAKE_BINARY_DIR}/bin"
        )

        target_link_libraries(test_${MODULE_NAME}
            PRIVATE
                daq::test_utils
                daq::${MODULE_NAME}
                GTest::gmock_main
                GTest::gtest_main
        )

        add_test(
            NAME                test_${MODULE_NAME}
            COMMAND             test_${MODULE_NAME}
            WORKING_DIRECTORY   "${CMAKE_SOURCE_DIR}"
        )

    endif()

endfunction()
