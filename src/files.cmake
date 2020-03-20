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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/pimpl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/thread.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/config/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/platforms/adaptation.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/containers/shared_vector.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/containers/flat_set.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/containers/flat_map.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/algorithm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/ref_counted.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/locks.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/function_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/type_traits.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/type_traits/enum_operators.hpp
    # Logger
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/utils/log/logger.hpp

    # meta/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/metatype.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/metatype_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/detail/metatype_descriptor_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/variant_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/detail/variant_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/callable.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/detail/callable_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/metadata.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/core/detail/metadata_impl.hpp

    # meta/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/base/metabase.hpp

    # meta/signal
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/signal/signal_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/signal/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/signal/detail/signal_impl.hpp

    # meta/property
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/property_decl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/property_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/property_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/property.hpp

    # meta/class
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/class/detail/metaclass_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/class/metaclass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/class/metaobject.hpp

    # meta/property/binding
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/binding/binding.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/binding/binding_normalizer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/binding/binding_group.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/binding/property_binding.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/meta/property/binding/expression_binding.hpp

    #event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/run_loop_sources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/run_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/event_handling_declarations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/event_handling/socket_notifier.hpp

    #modules
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/module.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/thread_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/thread_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/module/application.hpp

    # Mox core
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/mox_module.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/object.hpp

    # Private includes
    ${CMAKE_CURRENT_SOURCE_DIR}/include/logger_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/metabase_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/property_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/binding_p.hpp    
    ${CMAKE_CURRENT_SOURCE_DIR}/include/signal_p.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/log/logger.cpp

    # meta/core
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/core/metatype.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/core/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/core/variant.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/core/callable.cpp

    # meta/base
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/base/metabase.cpp
    # meta/signal
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/signal/signal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/signal/signal_storage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/signal/signal_connections.cpp

    # meta/property
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/property_storages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/property_type.cpp

    # meta/class
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/class/metaclass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/class/metaobject.cpp

    # meta/proeprty/binding
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/binding/binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/binding/binding_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/binding/property_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/meta/property/binding/expression_binding.cpp

    # Event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/event_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/socket_notifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/run_loop_sources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_handling/run_loop.cpp

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
