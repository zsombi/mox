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

if ("${TARGET}" STREQUAL "")
    message("TARGET not set.")
else()
    message("Configuring target " ${TARGET})
endif()

# MacOS stuff
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    target_link_libraries(${TARGET} "-framework Foundation -framework CoreFoundation")
    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()

# Linux stuff
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(${TARGET} PkgConfig::deps)
    target_link_libraries(${TARGET} ${LIBGLIB_LIBRARIES})
endif()
