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
        "ConfigManagerError.hh"
        "ConfigManagerError.cc"
        "DeviceList.hh"
        "DeviceList.cc"
        "TextWizardPage.hh"
        "TextWizardPage.cc"
        "TextWizardMenu.hh"
        "TextWizardMenu.cc"
        "ConfigurationWizard.hh"
        "ConfigurationWizard.cc"
        "NvStrapsConfig.cc"
        "${REBAR_DXE_DIRECTORY}/include/LocalAppConfig.h"
        "${REBAR_DXE_DIRECTORY}/include/StatusVar.h"
        "${REBAR_DXE_DIRECTORY}/include/DeviceRegistry.h"
        "${REBAR_DXE_DIRECTORY}/DeviceRegistry.c"
        "${REBAR_DXE_DIRECTORY}/include/NvStrapsConfig.h"
        "${REBAR_DXE_DIRECTORY}/NvStrapsConfig.c"
        "${REBAR_DXE_DIRECTORY}/StatusVar.c"
        "ReBarState.cc")
target_include_directories(NvStrapsReBar PRIVATE "${REBAR_DXE_DIRECTORY}/include" "${NvStrapsReBar_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(NvStrapsReBar PRIVATE "DXGI" "SetupAPI" "CfgMgr32")
    set_target_properties(NvStrapsReBar PROPERTIES LINK_FLAGS " /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" ")
endif()
