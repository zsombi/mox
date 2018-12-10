cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(configure-platform)

project(mox LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS on)

include(files.cmake)

add_definitions(-DMOX_LIBRARY)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 7)
set(PROJECT_VERSION_PATCH 1)

if (BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
else()
    add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()
