file(GLOB apps apps/*)

function(BuildApp app)

    file(GLOB_RECURSE sources ${app}/*.cpp)
    file(GLOB_RECURSE headers ${app}/*.h ${app}/*.hpp)

    add_executable(${app} ${sources} ${headers})

    set_target_properties(${app} PROPERTIES
        CXX_STANDARD            17
        CXX_STANDARD_REQUIRED   ON
        CXX_EXTENSIONS          ON)

    target_compile_definitions(${app} PRIVATE
        OPENDAQ_MODULES_DIR="${OPENDAQ_MODULES_DIR}")

    target_link_libraries(${app} PUBLIC
        daq::opendaq)

endfunction()

if(HBK_OPENDAQ_ENABLE_ADVANCED_RECORDER)
    BuildApp(advanced_recorder_test)
endif()
