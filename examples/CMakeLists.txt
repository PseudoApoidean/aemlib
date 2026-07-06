cmake_minimum_required(VERSION 3.20)

add_executable(aemlib_smoke_test smoke_test.c)
target_link_libraries(aemlib_smoke_test PRIVATE aemlib)

add_executable(aemlib_pubsub_demo pubsub_demo.c)
target_link_libraries(aemlib_pubsub_demo PRIVATE aemlib)

if (AEMLIB_BUILD_TESTS)
    add_test(NAME aemlib_smoke_test COMMAND aemlib_smoke_test)
    add_test(NAME aemlib_pubsub_demo COMMAND aemlib_pubsub_demo)
endif()
