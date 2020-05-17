cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(configure-target)

project(mox_lib VERSION 0.0.1 LANGUAGES CXX)

# get the files
include(files.cmake)

# BUILD_SHARED_LIBS drives the library type for add_Library
add_library(mox "")
target_sources(mox
    PRIVATE ${SOURCES} ${PLATFORM_SOURCES} ${PLATFORM_HEADERS} ${PRIVATE_HEADERS}
    PUBLIC ${HEADERS})
target_include_directories(mox
    PUBLIC ${MOX_HEADER_PATH}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
configure_target(mox)
