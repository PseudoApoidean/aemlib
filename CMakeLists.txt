cmake_minimum_required(VERSION 3.20)
project(aemlib LANGUAGES C)

# Options
option(AEMLIB_BUILD_TESTS "Build unit tests" ON)

# Library target
add_library(aemlib
    core/mqtt_core.c
)

target_include_directories(aemlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Subdirectories
add_subdirectory(proto)
add_subdirectory(platform)

# Tests
if (AEMLIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
