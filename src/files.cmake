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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/platforms/adaptation.hpp

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/metatype.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/metatype_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/detail/metatype_descriptor_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/variant_descriptor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/detail/variant_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/callable.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/detail/callable_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/metadata.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/core/detail/metadata_impl.hpp

    # meta/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/base/metabase.hpp

    # meta/signal
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/signal/signal_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/signal/signal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/signal/detail/signal_impl.hpp

    # meta/property
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/property_decl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/property_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/property_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/property.hpp

    # meta/class
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/class/detail/metaclass_impl.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/class/metaclass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/class/metaobject.hpp

    # meta/property/binding
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/binding/binding.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/binding/binding_normalizer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/binding/binding_group.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/binding/property_binding.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/meta/property/binding/expression_binding.hpp

    #event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/event_queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/run_loop_sources.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/run_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/event_handling_declarations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/event_handling/socket_notifier.hpp

    # process
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/process/thread_interface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/process/thread_data.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/process/thread_loop.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/process/application.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/process/applet.hpp

    # module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/module/module.hpp

    # Mox core
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/mox_module.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mox/core/object.hpp
    )

set(PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/logger_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/metabase_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/property_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/binding_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/signal_p.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/process_p.hpp
    )

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/log/logger.cpp

    # meta/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/core/metatype.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/core/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/core/variant.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/core/callable.cpp

    # meta/base
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/base/metabase.cpp
    # meta/signal
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/signal/signal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/signal/signal_storage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/signal/signal_connections.cpp

    # meta/property
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/property_storages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/property_type.cpp

    # meta/class
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/class/metaclass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/class/metaobject.cpp

    # meta/proeprty/binding
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/binding/binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/binding/binding_group.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/binding/property_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/meta/property/binding/expression_binding.cpp

    # Event handling
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_handling/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_handling/event_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_handling/socket_notifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_handling/run_loop_sources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/event_handling/run_loop.cpp

    # process
    ${CMAKE_CURRENT_SOURCE_DIR}/core/process/thread_interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/process/thread_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/process/thread_loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/process/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/process/applet.cpp

    # object
    ${CMAKE_CURRENT_SOURCE_DIR}/core/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/object.cpp

    # module
    ${CMAKE_CURRENT_SOURCE_DIR}/core/module/modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mox_module.cpp
    )

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(PLATFORM_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/event_dispatcher.h
        )
    set(PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/run_loop.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/post_event_source.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/socket_notifier_source.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/timer_source.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux_x86/idle_source.cc
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
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/darwin/idle_source.mm
        )
endif()
