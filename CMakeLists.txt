cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake.modules" CACHE STRING "module-path")

option(MOX_TESTS "Build Mox unit tests." OFF)
option(MOX_ENABLE_LOGS "Enable logs." OFF)
option(BUILD_SHARED_LIBS "Build shared libraries." ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(MOX_SOURCES ${CMAKE_SOURCE_DIR})
set(MOX_BUILD_PATH ${CMAKE_BINARY_DIR})
include(configure-platform)

add_subdirectory(src)

if (MOX_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

