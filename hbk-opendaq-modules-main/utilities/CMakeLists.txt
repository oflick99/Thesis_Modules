add_library(hbk_opendaq_utils INTERFACE)
add_library(hbk::opendaq_utils ALIAS hbk_opendaq_utils)

target_include_directories(hbk_opendaq_utils INTERFACE include)
