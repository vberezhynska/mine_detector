cmake_minimum_required(VERSION 3.20)
project(ballistics CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Libraries -----
add_library(tracker 
    src/dto/point.cpp
    # Add other source files here as you create them, e.g.:
    # src/controller.cpp
)

# Explicitly declare where your header files live
target_include_directories(tracker PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/external>
)

# ---- Main executable -----
add_executable(mine_tracker 
    src/main.cpp
)

# Link your tracker library to the executable
target_link_libraries(mine_tracker PRIVATE tracker)