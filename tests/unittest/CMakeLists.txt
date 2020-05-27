#
# Copyright (C) 2017-2020 bitWelder
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

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/../cmake.modules" CACHE STRING "module-path")

include(configure-target)
#set(CMAKE_SKIP_RPATH ON)
project(test VERSION 0.0.1)

set(SOURCES
    test_enumerate_metatypes.cpp
    test_flatset.cpp
    test_flatmap.cpp
    test_metatypes.cpp
    test_converters.cpp
    test_argument.cpp
    test_callables.cpp
    test_object.cpp
    test_event.cpp
    test_event_queue.cpp
    test_runloophooks.cpp
    test_event_handling.cpp
    test_threads.cpp
    test_applets.cpp
    test_metakernel_signals.cpp
    test_metakernel_properties.cpp
    )

add_executable(unittest ${SOURCES} ${TEST_FRAMEWORK})
target_link_libraries(unittest PRIVATE gtest_main mox)
target_include_directories(unittest PRIVATE ${CMAKE_SOURCE_DIR}/tests)
configure_target(unittest)

add_test(NAME unittest COMMAND unittest "--gtest_output=xml:unittest.xml"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
