include_guard(GLOBAL)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cxx_std_lib.pcm"
        COMMAND
            "${CMAKE_CXX_COMPILER}" "${CMAKE_CXX_FLAGS}" "-v"
                "-std=gnu++23"
                "-fexperimental-library"
                "-fretain-comments-from-system-headers"
                "-x" "c++" "-Xclang" "-emit-module" "-fmodule-name=std"
                "-o" "${CMAKE_CURRENT_BINARY_DIR}/cxx_std_lib.pcm" "-c"
                "${CMAKE_CURRENT_SOURCE_DIR}/cxx_std_lib.modulemap"
            MAIN_DEPENDENCY "cxx_std_lib.modulemap" DEPENDS "cxx_std_lib.hh")

    add_library(CxxModuleStd INTERFACE)
    target_compile_options(CxxModuleStd INTERFACE
	"-Wno-deprecated-anon-enum-enum-conversion"
	"-std=gnu++23"
	"-fexperimental-library"
	"-x" "c++"
	"-Wno-dangling-else"
	"-Wno-unqualified-std-cast-call"
	"-Wno-switch"
	"-fretain-comments-from-system-headers"
	"-fmodule-file=${CMAKE_CURRENT_BINARY_DIR}/cxx_std_lib.pcm")
    target_sources(CxxModuleStd INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/cxx_std_lib.pcm")
    target_compile_features(CxxModuleStd INTERFACE cxx_std_23)
    add_library(CxxModule::Std ALIAS CxxModuleStd)
elseif(MSVC)
    if(NOT MSVC_CXX_STD_MODULE_SOURCE_FILE)
        if(DEFINED ENV{VCToolsInstallDir})
            set(MSVC_CXX_STD_MODULE_SOURCE_FILE "$ENV{VCToolsInstallDir}/modules/std.ixx")
        else()
            find_program(VSWHERE_EXECUTABLE NAMES vswhere HINTS "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer" REQUIRED)

	    if(MSVC)
                execute_process(
                    COMMAND "${VSWHERE_EXECUTABLE}" "-path" "${CMAKE_CXX_COMPILER}" "-find" "VC/Tools/MSVC/*/modules/std.ixx"
                    COMMAND_ECHO STDOUT
                    OUTPUT_VARIABLE MSVC_CXX_STD_MODULE_SOURCE_FILE
                    ECHO_OUTPUT_VARIABLE
                    OUTPUT_STRIP_TRAILING_WHITESPACE
		    RESULT_VARIABLE VSWHERE_EXIT_CODE
                    COMMAND_ERROR_IS_FATAL ANY)
		if (MSVC_CXX_STD_MODULE_SOURCE_FILE STREQUAL "" OR NOT VSWHERE_EXIT_CODE STREQUAL "0")
		    execute_process(
			COMMAND "${VSWHERE_EXECUTABLE}" "-latest"
			    "-requires" "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
			    "-requires" "Microsoft.VisualStudio.Component.Windows11SDK.*"
			    "-products" "Microsoft.VisualStudio.Product.Community"
			    "-find" "VC/Tools/MSVC/*/modules/std.ixx"
			COMMAND_ECHO STDOUT
			OUTPUT_VARIABLE MSVC_CXX_STD_MODULE_SOURCE_FILE
			ECHO_OUTPUT_VARIABLE
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ERROR_IS_FATAL ANY)
		endif()
	    endif()
        endif()
    endif()

    cmake_path(CONVERT "${MSVC_CXX_STD_MODULE_SOURCE_FILE}" TO_CMAKE_PATH_LIST MSVC_CXX_STD_MODULE_SOURCE_FILE NORMALIZE)

    if (NOT MSVC_CXX_STD_MODULE_SOURCE_FILE OR NOT EXISTS "${MSVC_CXX_STD_MODULE_SOURCE_FILE}")
        message(FATAL_ERROR "Unable to find C++ module source file std.ixx for Visual C++ standard libary")
    endif()

    # add_library(CxxModuleStd OBJECT)
    # cmake_path(GET MSVC_CXX_STD_MODULE_SOURCE_FILE PARENT_PATH MSVC_CXX_STD_MODULE_DIR)
    # target_sources(CxxModuleStd PUBLIC FILE_SET std_cxx_module TYPE CXX_MODULES BASE_DIRS "${MSVC_CXX_STD_MODULE_DIR}" FILES "${MSVC_CXX_STD_MODULE_SOURCE_FILE}")
    # target_compile_features(CxxModuleStd PUBLIC cxx_std_23)

    add_library(CxxModuleStd INTERFACE)
    cmake_path(GET MSVC_CXX_STD_MODULE_SOURCE_FILE PARENT_PATH MSVC_CXX_STD_MODULE_DIR)
    target_sources(CxxModuleStd INTERFACE "${MSVC_CXX_STD_MODULE_SOURCE_FILE}")
    target_compile_features(CxxModuleStd INTERFACE cxx_std_23)

    add_library(CxxModule::Std ALIAS CxxModuleStd)
endif()
