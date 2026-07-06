cmake_minimum_required(VERSION 3.20)
project(aemlib LANGUAGES C)

# Options
option(AEMLIB_BUILD_TESTS "Build unit tests" ON)
option(AEMLIB_BUILD_EXAMPLES "Build examples" ON)

# Library target
add_library(aemlib
    core/core.c
    core/client.c
    core/log.c
    core/status.c
    proto/proto.c
    platform/virtual_transport.c
)

target_include_directories(aemlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

# Tests
if (AEMLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if (AEMLIB_BUILD_EXAMPLES)
    if (NOT AEMLIB_BUILD_TESTS)
        enable_testing()
    endif()
    add_subdirectory(examples)
endif()
