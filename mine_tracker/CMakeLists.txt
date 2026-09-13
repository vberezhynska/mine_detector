cmake_minimum_required(VERSION 3.20)
project(mine_tracker CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Dependencies ----
set(CROW_USE_BOOST OFF CACHE BOOL "Disable Boost for Crow" FORCE)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "Disable Crow examples" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "Disable Crow tests" FORCE)

add_compile_definitions(ASIO_STANDALONE)

include(FetchContent)

# Fetch Standalone Asio
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Tell Crow where Asio headers are
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Asio include dir" FORCE)

# Fetch Crow
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(Crow)

find_package(Threads REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Asio library not found! Install it on Pi via: sudo apt install libasio-dev")
endif()

# ---- Libraries ----
add_library(tracker 
    src/udp_server.cpp
    src/http_server.cpp
)

# Explicitly declare where your header files live
target_include_directories(tracker PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/external>
    ${ASIO_INCLUDE_DIR} # Added so Crow can locate asio.hpp in tracker
)

# Link Crow and Threads to the tracker library
target_link_libraries(tracker PUBLIC 
    Crow::Crow
    Threads::Threads # Required by Crow's thread pool
)

# ---- Main Executable ----
add_executable(mine_tracker 
    src/main.cpp
)

# Link your tracker library to the executable
target_link_libraries(mine_tracker PRIVATE tracker)