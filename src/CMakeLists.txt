cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
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
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${PLATFORM_SOURCES} ${HEADERS} ${PLATFORM_HEADERS})
else()
    add_library(${PROJECT_NAME} ${SOURCES} ${PLATFORM_SOURCES} ${HEADERS} ${PLATFORM_HEADERS})
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_link_libraries(mox ${LIBGLIB_LIBRARIES})
endif()

# platform specific packages
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(mox PkgConfig::deps)
endif()

# MacOS stuff
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    target_link_libraries(mox "-framework Foundation -framework CoreFoundation")
    set_target_properties(mox PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()

target_include_directories(mox PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
