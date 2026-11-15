#-----------------------------------------Default from Tutorial--------------------------------------------------------
make_minimum_required(VERSION 3.24)
project(tcp_con_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

#-------------------------------------------------------------------------------------------------

# Compile the actual source as C
add_library(tcp_con_under_test OBJECT ../tcp_con.c)
# The following adds compiler definitions which are used when compiling the targets
# PRIVATE - This means that only tcp_con_under_test will inherit these
target_compile_definitions(tcp_con_under_test PRIVATE TCP_CON_TEST_BUILD)
# Add compiler flags, we will want to include test_io
# This will compile our target: tcp_con_under_test will extra warnings
target_compile_options(tcp_con_under_test PRIVATE -Wall -Wextra -include "${CMAKE_CURRENT_SOURCE_DIR}/test_io.h")
# Add executable named: tcp_con_test
#-----------------------------------------Default from Tutorial--------------------------------------------------------
add_executable(tcp_con_test tcp_con_test.cpp)
# This will compile our target: tcp_con_under_test will extra warnings
target_compile_options(tcp_con_test PRIVATE -Wall -Wextra)
# Link the executable: tcp_con_test to tcp_con_under_test library
target_link_libraries(tcp_con_test PRIVATE tcp_con_under_test GTest::gtest_main)
#-------------------------------------------------------------------------------------------------

# The following option is a convenient way to test for code coverage, Defaults to OFF
option(ENABLE_COVERAGE "Measure tcp_con.c code coverage" OFF)
if(ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Coverage requires Clang for both C and C++")
    endif()
    target_compile_options(tcp_con_under_test PRIVATE -O0 -g -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(tcp_con_test PRIVATE -fprofile-instr-generate)
endif()

enable_testing()
include(GoogleTest)
gtest_discover_tests(tcp_con_test DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 10)