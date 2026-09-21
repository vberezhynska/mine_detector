cmake_minimum_required(VERSION 3.20)
project(mine_tracker CXX C) # C kept for sqlite3.c compilation

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

# ===== Asio ===== 
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Tell Crow where Asio headers are
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Asio include dir" FORCE)
# ===== END Asio =====

# ===== Crow (HTTP) ===== 
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(Crow)
# ===== END Crow =====

# ===== SQLite3 ===== 
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)

# Build SQLite as a target directly from fetched sources
add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
target_compile_definitions(sqlite3_lib PRIVATE
    SQLITE_ENABLE_JSON1
    SQLITE_ENABLE_RTREE
)
# ===== END SQLite3 =====

# ===== Boost Headers (for Boost.Geometry) ===== 
FetchContent_Declare(
    boost_source
    URL https://archives.boost.io/release/1.84.0/source/boost_1_84_0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(boost_source)
if(NOT boost_source_POPULATED)
    message(STATUS "Fetching Boost source headers (may take a minute on initial setup)...")
    FetchContent_MakeAvailable(boost_source)
endif()

add_library(boost_headers INTERFACE)
target_include_directories(boost_headers INTERFACE ${boost_source_SOURCE_DIR})
# ===== END Boost Headers =====

# ===== MavLink Headers ===== 
FetchContent_Declare(
    mavlink_c_v2
    GIT_REPOSITORY https://github.com/mavlink/c_library_v2.git
    GIT_TAG        master  
    GIT_SHALLOW    TRUE    
)

FetchContent_GetProperties(mavlink_c_v2)
if(NOT mavlink_c_v2_POPULATED)
    FetchContent_Populate(mavlink_c_v2)
endif()

if(NOT TARGET mavlink)
    add_library(mavlink INTERFACE)
    target_include_directories(mavlink INTERFACE
        ${mavlink_c_v2_SOURCE_DIR}
        ${mavlink_c_v2_SOURCE_DIR}/common
    )
endif()
# ===== END MavLink Headers =====

find_package(Threads REQUIRED)

# ---- Libraries ----
add_library(tracker 
    src/cli_args.cpp    
    src/udp_server.cpp
    src/http_server.cpp
    src/db_manager.cpp
    src/flags.cpp
    src/mine_alert_manager.cpp
    src/confidence_engine.cpp
    src/mavlink_broadcaster.cpp
)

# Explicitly declare where your header files live
target_include_directories(tracker PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/external>
    ${ASIO_INCLUDE_DIR}
)

# Link Crow, Threads, SQLite, and Boost headers to the tracker library
target_link_libraries(tracker PUBLIC 
    Crow::Crow
    Threads::Threads
    sqlite3_lib
    boost_headers
    mavlink
)

# ---- Main Executable ----
add_executable(mine_tracker 
    src/main.cpp
)

target_compile_definitions(mine_tracker PRIVATE
    DB_PATH="${CMAKE_SOURCE_DIR}/mines.db"
)

# Link tracker library to the executable
target_link_libraries(mine_tracker PRIVATE tracker)

target_compile_options(mine_tracker PRIVATE -Wno-psabi)