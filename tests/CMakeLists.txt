cmake_minimum_required(VERSION 3.20)

# Unity is fetched for its sources only. SOURCE_SUBDIR names a directory that
# does not exist so its CMakeLists is never added: Unity installs itself
# unconditionally, and cmake --install would otherwise drop libunity.a, its
# headers and a unityConfig package into AEMlib's install prefix.
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
    SOURCE_SUBDIR do-not-add-unitys-cmakelists
)
FetchContent_MakeAvailable(unity)

# No -pedantic-errors: Unity is not pedantic-clean.
add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)
target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src)

# Test executable
add_executable(aemlib_tests
    main.c
)

target_link_libraries(aemlib_tests
    PRIVATE
        aemlib
        aemlib_virtual_transport
        unity
)

# The tests are an internal consumer: they include core.h, proto.h and
# virtual_transport.h directly. Those directories are PRIVATE on aemlib, so
# they are requested here rather than inherited.
target_include_directories(aemlib_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
        ${CMAKE_CURRENT_SOURCE_DIR}/../proto
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform
        ${unity_SOURCE_DIR}/src
)

# Add test
add_test(NAME aemlib_unit_tests COMMAND aemlib_tests)