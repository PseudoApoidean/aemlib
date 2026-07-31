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
)

# The archive is libaem.a, not libaemlib.a: the lib prefix already supplies the
# "lib" in AEMlib, the same way zlib ships as libz.a. Only the filename changes;
# the target, package and header directory stay aemlib.
set_target_properties(aemlib PROPERTIES
    OUTPUT_NAME aem
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# The properties above only select -std=c99; GCC still accepts most GNU
# extensions in that mode. -pedantic-errors is what rejects them. PRIVATE
# because the tests link Unity, which is not pedantic-clean.
target_compile_options(aemlib PRIVATE -pedantic-errors)

# include/ is the public API. core/, proto/ and platform/ are PRIVATE so a
# consumer cannot include them and pin an interface that is meant to move.
# Internal consumers (tests, and the fake broker in pubsub_demo) ask for these
# directories explicitly on their own targets.
target_include_directories(aemlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/proto
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

# Test double shared by the tests and pubsub_demo; never installed.
if (AEMLIB_BUILD_TESTS OR AEMLIB_BUILD_EXAMPLES)
    add_library(aemlib_virtual_transport STATIC platform/virtual_transport.c)

    set_target_properties(aemlib_virtual_transport PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )

    target_compile_options(aemlib_virtual_transport PRIVATE -pedantic-errors)

    target_link_libraries(aemlib_virtual_transport PUBLIC aemlib)

    target_include_directories(aemlib_virtual_transport
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/platform
    )
endif()

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
