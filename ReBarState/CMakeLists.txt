cmake_minimum_required(VERSION 3.28)

project("NvStrapsReBar" LANGUAGES C CXX)

include(cmake/LocalModulePath.cmake)
include(CxxStdModule)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(NOT REBAR_DXE_DIRECTORY)
    cmake_path(SET REBAR_DXE_DIRECTORY NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../ReBarDxe")
endif()

add_executable(NvStrapsReBar
        "${REBAR_DXE_DIRECTORY}/include/LocalAppConfig.h"
        "${REBAR_DXE_DIRECTORY}/include/DeviceRegistry.h"
        "${REBAR_DXE_DIRECTORY}/DeviceRegistry.c"
        "${REBAR_DXE_DIRECTORY}/include/EfiVariable.h"
        "${REBAR_DXE_DIRECTORY}/EfiVariable.c"
        "${REBAR_DXE_DIRECTORY}/include/NvStrapsConfig.h"
        "${REBAR_DXE_DIRECTORY}/NvStrapsConfig.c"
        "${REBAR_DXE_DIRECTORY}/include/StatusVar.h"
        "${REBAR_DXE_DIRECTORY}/StatusVar.c"
        "ReBarState.cc")

set_property(SOURCE
    "${REBAR_DXE_DIRECTORY}/DeviceRegistry.c"
    "${REBAR_DXE_DIRECTORY}/EfiVariable.c"
    "${REBAR_DXE_DIRECTORY}/NvStrapsConfig.c"
    "${REBAR_DXE_DIRECTORY}/StatusVar.c"

    # for clang to compile as C++, but not include C++ headers and libraries
    APPEND PROPERTY COMPILE_DEFINITIONS "NVSTRAPS_DXE_DRIVER"
)

target_sources(NvStrapsReBar
    PRIVATE "cxx_std_lib.hh"
    PRIVATE FILE_SET CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    "LocalAppConfig.ixx"
    "StatusVar.ixx"
    "DeviceRegistry.ixx"
    "NvStrapsWinAPI.ixx"
    "NvStrapsDXGI.ixx"
    "WinApiError.ixx"
    "ConfigManagerError.ixx"
    "DeviceList.ixx"
    "TextWizardPage.ixx"
    "NvStrapsConfig.ixx"
    "TextWizardMenu.ixx"
    "ConfigurationWizard.ixx"
)

target_compile_features(NvStrapsReBar PRIVATE c_std_17 cxx_std_23)
target_include_directories(NvStrapsReBar PRIVATE "${REBAR_DXE_DIRECTORY}/include" "${NvStrapsReBar_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(NvStrapsReBar PRIVATE CxxModule::Std)
    target_link_libraries(NvStrapsReBar PRIVATE "DXGI" "SetupAPI" "CfgMgr32")

    if(MSVC)
        set_target_properties(NvStrapsReBar PROPERTIES LINK_FLAGS " /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" ")
    endif()
endif()

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
