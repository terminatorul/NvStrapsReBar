# CMakeList.txt : CMake project for ReBarState, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project("NvStrapsReBar")
set(CMAKE_CXX_STANDARD 23)

if(NOT REBAR_DXE_DIRECTORY)
    cmake_path(SET REBAR_DXE_DIRECTORY NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../ReBarDxe")
endif()

add_executable(NvStrapsReBar
        "WinApiError.hh"
        "WinApiError.cc"
        "DeviceList.hh"
        "DeviceList.cc"
        "${REBAR_DXE_DIRECTORY}/include/DeviceRegistry.hh"
        "${REBAR_DXE_DIRECTORY}/DeviceRegistry.cc"
        "${REBAR_DXE_DIRECTORY}/include/NvStrapsPciConfig.hh"
        "${REBAR_DXE_DIRECTORY}/NvStrapsPciConfig.cc"
        "ReBarState.cpp")
target_include_directories(NvStrapsReBar PRIVATE "${REBAR_DXE_DIRECTORY}/include" "${NvStrapsReBar_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(NvStrapsReBar PRIVATE "DXGI" "SetupAPI")
    set_target_properties(NvStrapsReBar PROPERTIES LINK_FLAGS " /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" ")
endif()
