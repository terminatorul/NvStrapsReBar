# CMakeList.txt : CMake project for ReBarState, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ReBarState")
set(CMAKE_CXX_STANDARD 23)

add_executable (NvStrapsReBar "DeviceList.hh" "DeviceList.cc" "ReBarState.cpp")

if(WIN32)
    target_link_libraries(NvStrapsReBar PRIVATE "DXGI" "SetupAPI")
    set_target_properties(NvStrapsReBar PROPERTIES LINK_FLAGS " /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" ")
endif()
