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

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/globals.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metatype.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/detail/metatype_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metaclass.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metadata_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metatype.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metaclass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/variant.cpp
    )
