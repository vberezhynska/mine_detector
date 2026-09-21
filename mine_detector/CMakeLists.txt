cmake_minimum_required(VERSION 3.20)

# Tell ESP-IDF to register the 'src' directory as a component directory
set(EXTRA_COMPONENT_DIRS "src")

# Include standard ESP-IDF build system macros
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(mine_detector)