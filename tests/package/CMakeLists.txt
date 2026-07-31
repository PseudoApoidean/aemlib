cmake_minimum_required(VERSION 3.20)
project(aemlib_package_consumer LANGUAGES C)

# No version requested: this proves the package resolves and links, and pinning
# a number here would fail the CI job on the next version bump instead.
find_package(aemlib REQUIRED)

add_executable(consumer consumer.c)
target_link_libraries(consumer PRIVATE aemlib::aemlib)
