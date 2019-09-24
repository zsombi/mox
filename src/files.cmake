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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/platform_config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/pimpl.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/globals.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/locks.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/function_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/type_traits/enum_operators.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metatype.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metatype_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/variant_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/detail/variant_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metaclass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metaobject.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/callable.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/detail/callable_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/detail/metaclass_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/metadata.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/metadata/detail/metadata_impl.hpp

    # Signal handling
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/signal/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/signal/detail/signal_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/signal/signal_host.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metadata_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metatype.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metaclass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/metaobject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/variant.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata/callable.cpp
    # Signal
    ${CMAKE_CURRENT_SOURCE_DIR}/signal/signal_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/signal/signal.cpp
    )
