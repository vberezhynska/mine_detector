cmake_minimum_required(VERSION 3.20)
project(gps_decoder_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Fetch GoogleTest automatically
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# 2. Add test executable sourcing implementation from ../src
add_executable(
  run_gps_tests
  test_gps_decoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/gps_decoder.cpp
)

# 3. Expose the ../include directory for header resolution
target_include_directories(
  run_gps_tests 
  PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 4. Define macro for host ESP_LOG mocks
target_compile_definitions(run_gps_tests PRIVATE HOST_TESTING)

# 5. Link GoogleTest main
target_link_libraries(
  run_gps_tests
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_gps_tests)