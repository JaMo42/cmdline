cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

project(cmdline)
include_directories(include/cmdline)

add_library(cmdline SHARED source/cmdline.cpp)
add_library(cmdline_static source/cmdline.cpp)

enable_testing()
find_package(GTest MODULE REQUIRED)

project(cmdline_tests)
file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(cmdline_tests ${TEST_SOURCES})
set_target_properties(cmdline_tests PROPERTIES OUTPUT_NAME "run_tests")
target_link_libraries(cmdline_tests PRIVATE GTest::GTest GTest::Main cmdline)
add_test(AllTestsInMain cmdline_tests)
