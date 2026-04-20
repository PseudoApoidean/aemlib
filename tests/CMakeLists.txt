cmake_minimum_required(VERSION 3.20)

# Include Unity testing framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Test executable
add_executable(aemlib_tests
    main.c
)

target_link_libraries(aemlib_tests
    PRIVATE
        aemlib
        unity
)

target_include_directories(aemlib_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
        ${unity_SOURCE_DIR}/src
)

# Add test
add_test(NAME aemlib_unit_tests COMMAND aemlib_tests)