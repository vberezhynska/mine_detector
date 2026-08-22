cmake_minimum_required(VERSION 3.20)
project(section2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Fetch and Configure GoogleTest -----
enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    include(FetchContent)
    FetchContent_Declare(
        googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
                       DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(googletest)
endif()
include(GoogleTest)

add_subdirectory(mine_tracker)