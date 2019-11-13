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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/deftypes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/pimpl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/thread.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/platforms/adaptation.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/globals.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/locks.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/flat_set.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/flat_map.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/containers.hpp
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

    #event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_handler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/socket_notifier.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_sources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_dispatcher.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_handling_declarations.hpp

    #modules
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/module.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/thread_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/thread_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/application.hpp

    # Mox core
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/mox_module.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/object.hpp
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
    # Event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/socket_notifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_sources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_dispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_loop.cpp

    # object
    ${CMAKE_CURRENT_SOURCE_DIR}/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/object.cpp

    #modules
    ${CMAKE_CURRENT_SOURCE_DIR}/module/modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/module/thread_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/module/thread_loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/module/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mox_module.cpp
    )

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(PLATFORM_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/event_dispatcher.h
        )
    set(PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/event_dispatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/post_event_source.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/socket_notifier_source.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/timer_source.cpp
        )
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(PLATFORM_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/mac_util.h
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/event_dispatcher.h
        )
    set(PLATFORM_SOURCES        
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/event_dispatcher.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/timer_source.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/post_event_source.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/socket_notifier.mm
        )
endif()
