cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(configure-platform)

set(TARGET mox)

project(${TARGET} LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS on)

include(files.cmake)

add_definitions(-DMOX_LIBRARY)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 7)
set(PROJECT_VERSION_PATCH 1)

if (BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${PLATFORM_SOURCES} ${HEADERS} ${PLATFORM_HEADERS})
else()
    add_library(${PROJECT_NAME} ${SOURCES} ${PLATFORM_SOURCES} ${HEADERS} ${PLATFORM_HEADERS})
endif()

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
include(configure-target)
