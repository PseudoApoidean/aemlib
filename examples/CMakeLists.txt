cmake_minimum_required(VERSION 3.20)

add_executable(aemlib_smoke_test smoke_test.c)
target_link_libraries(aemlib_smoke_test PRIVATE aemlib)

add_executable(aemlib_pubsub_demo pubsub_demo.c)
target_link_libraries(aemlib_pubsub_demo PRIVATE aemlib)

# Talks to a real public broker over TCP - not hermetic, so it's built but
# deliberately not registered as a ctest (would make the suite flaky/networked).
add_executable(aemlib_mqtt_broker_demo mqtt_broker_demo.c posix_tcp_transport.c)
target_link_libraries(aemlib_mqtt_broker_demo PRIVATE aemlib)

if (AEMLIB_BUILD_TESTS)
    add_test(NAME aemlib_smoke_test COMMAND aemlib_smoke_test)
    add_test(NAME aemlib_pubsub_demo COMMAND aemlib_pubsub_demo)
endif()
