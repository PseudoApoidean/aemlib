cmake_minimum_required(VERSION 3.20)

add_executable(aemlib_smoke_test smoke_test.c)
target_link_libraries(aemlib_smoke_test PRIVATE aemlib)

if (AEMLIB_BUILD_TESTS)
    add_test(NAME aemlib_smoke_test COMMAND aemlib_smoke_test)
endif()
