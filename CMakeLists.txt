cmake_minimum_required(VERSION 3.20)

# The public header owns the version. Reading it here rather than repeating the
# number in project() keeps the package version and AEMLIB_VERSION_* from
# drifting apart.
file(STRINGS include/aemlib/aemlib.h aemlib_version_line
    REGEX "^#define AEMLIB_VERSION_(MAJOR|MINOR|PATCH) ")
string(REGEX MATCHALL "[0-9]+" aemlib_version_parts "${aemlib_version_line}")
list(JOIN aemlib_version_parts "." aemlib_version)

project(aemlib VERSION ${aemlib_version} LANGUAGES C)

include(GNUInstallDirs)

# Installing is on by default for a standalone build and off when AEMlib is
# somebody's subdirectory, where its install rules would write into a prefix the
# parent never meant to fill. PROJECT_IS_TOP_LEVEL would say this directly but
# needs CMake 3.21.
set(aemlib_is_top_level OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(aemlib_is_top_level ON)
endif()

# Options
option(AEMLIB_BUILD_TESTS "Build unit tests" ON)
option(AEMLIB_BUILD_EXAMPLES "Build examples" ON)
option(AEMLIB_INSTALL "Generate install and package rules" ${aemlib_is_top_level})

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

# In-tree consumers link the same aemlib::aemlib name that find_package provides,
# so a project can switch between add_subdirectory and an installed package
# without touching its link lines.
add_library(aemlib::aemlib ALIAS aemlib)

# include/ is the public API. core/, proto/ and platform/ are PRIVATE so a
# consumer cannot include them and pin an interface that is meant to move.
# Internal consumers (tests, and the fake broker in pubsub_demo) ask for these
# directories explicitly on their own targets.
target_include_directories(aemlib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

# Install
if (AEMLIB_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS aemlib
        EXPORT aemlibTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # include/aemlib only. The internal directories are PRIVATE above and must
    # not reach the installed package.
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/aemlib
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT aemlibTargets
        FILE aemlibTargets.cmake
        NAMESPACE aemlib::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aemlib
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/aemlibConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/aemlibConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aemlib
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/aemlibConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/aemlibConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/aemlibConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aemlib
    )
endif()
