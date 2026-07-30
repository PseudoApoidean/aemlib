cmake_minimum_required(VERSION 3.20)

add_executable(aemlib_smoke_test smoke_test.c)
target_link_libraries(aemlib_smoke_test PRIVATE aemlib)

# The only example that reaches past the public API: its fake broker encodes
# CONNACK/SUBACK by hand and drives the loopback transport, neither of which a
# consumer can do. smoke_test and mqtt_broker_demo get no internal directories
# on purpose - they only compile if the public API is sufficient on its own.
add_executable(aemlib_pubsub_demo pubsub_demo.c)
target_link_libraries(aemlib_pubsub_demo PRIVATE aemlib)
target_include_directories(aemlib_pubsub_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../proto
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)

# Talks to a real public broker over TCP - not hermetic, so it's built but
# deliberately not registered as a ctest (would make the suite flaky/networked).
add_executable(aemlib_mqtt_broker_demo mqtt_broker_demo.c posix_tcp_transport.c)
target_link_libraries(aemlib_mqtt_broker_demo PRIVATE aemlib)

if (AEMLIB_BUILD_TESTS)
    add_test(NAME aemlib_smoke_test COMMAND aemlib_smoke_test)
    add_test(NAME aemlib_pubsub_demo COMMAND aemlib_pubsub_demo)
endif()
