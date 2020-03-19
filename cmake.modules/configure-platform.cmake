#
# Copyright (C) 2017-2018 bitWelder
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see
# <http://www.gnu.org/licenses/>
#

if (MOX_CONFIG_PATFORM)
    return()
endif()
set(MOX_CONFIG_PATFORM true)

set(MOX_LIB_PATH ${PROJECT_BINARY_DIR}/lib/${CMAKE_SYSTEM_NAME})
set(MOX_BIN_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Werror -Wall -W -fPIC -pthread")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Winconsistent-missing-override -fvisibility=hidden")
endif()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wno-subobject-linkage")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MOX_LIB_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MOX_LIB_PATH})

if (MOX_ENABLE_LOGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMOX_ENABLE_LOGS")
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMOX_HOST_LINUX")
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMOX_HOST_MACOSX")
endif()

message(STATUS "System  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build   : ${CMAKE_BUILD_TYPE}")
message(STATUS "LibDir  : ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "ArcDir  : ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message(STATUS "cxx     : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "cxxflags: ${CMAKE_CXX_FLAGS}")

