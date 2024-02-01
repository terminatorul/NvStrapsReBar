include_guard(DIRECTORY)

# Make the local cmake/ subdirectory available in
# CMAKE_MODULE_PATH

if(NOT "${CMAKE_CURRENT_SOURCE_DIR}/cmake" IN_LIST CMAKE_MODULE_PATH)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
endif()
