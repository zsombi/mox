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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(gtest)

set(SOURCES
    test_framework.h
    test_main.cpp
    test_metatypes.cpp
    test_metaclass.cpp
    test_variant.cpp
    )
add_executable(unittest ${SOURCES})
target_link_libraries(unittest gtest_main mox)

add_test(NAME unittest COMMAND unittest "--gtest_output=xml:unittest.xml"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
