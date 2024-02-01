include(CTest)
include(CxxStdModule)

cmake_minimum_required(VERSION 3.27)

create_test_sourcelist(NVSTRAPS_REBAR_TEST_SOURCES TestNvStrapsReBar.cc TestNvStrapsConfig.cc)

set(TEST_NVSTRAPS_REBAR_SOURCES
        "${REBAR_DXE_DIRECTORY}/include/EfiVariable.h"
        "${REBAR_DXE_DIRECTORY}/include/StatusVar.h"
        "${REBAR_DXE_DIRECTORY}/include/DeviceRegistry.h"
        "${REBAR_DXE_DIRECTORY}/include/NvStrapsConfig.h"
        "${REBAR_DXE_DIRECTORY}/EfiVariable.c"
        "${REBAR_DXE_DIRECTORY}/StatusVar.c"
        "${REBAR_DXE_DIRECTORY}/DeviceRegistry.c"
        "${REBAR_DXE_DIRECTORY}/NvStrapsConfig.c"
	"${NvStrapsReBar_SOURCE_DIR}/LocalAppConfig.ixx"
        "${NvStrapsReBar_SOURCE_DIR}/WinApiError.ixx"
	"${NvStrapsReBar_SOURCE_DIR}/NvStrapsWinAPI.ixx"
	"${NvStrapsReBar_SOURCE_DIR}/DeviceRegistry.ixx"
        "${NvStrapsReBar_SOURCE_DIR}/NvStrapsConfig.ixx"

        TestNvStrapsReBar.cc
        TestNvStrapsConfig.cc
        )

add_executable(TestNvStrapsReBar ${TEST_NVSTRAPS_REBAR_SOURCES})
target_link_libraries(TestNvStrapsReBar PRIVATE SetupAPI $<TARGET_PROPERTY:NvStrapsReBar,TARGET_LINK_LIBRARIES>)

target_include_directories(TestNvStrapsReBar PRIVATE $<TARGET_PROPERTY:NvStrapsReBar,INCLUDE_DIRECTORIES>)

list(POP_FRONT NVSTRAPS_REBAR_TEST_SOURCES)

foreach(TEST_FILE IN LISTS NVSTRAPS_REBAR_TEST_SOURCES)
    cmake_path(GET TEST_FILE STEM TEST_NAME)
    add_test(NAME "${TEST_NAME}" COMMAND TestNvStrapsReBar "${TEST_NAME}")
endforeach()
