/*
 * Copyright (C) 2017-2019 bitWelder
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>
 */
#ifndef EVENT_HANDLING_DECLARATIONS_HPP
#define EVENT_HANDLING_DECLARATIONS_HPP

#include <memory>
#include <mox/config/platform_config.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

class Event;
using EventPtr = std::unique_ptr<Event>;

class EventQueue;
using EventQueueSharedPtr = std::shared_ptr<EventQueue>;

class EventHandlingProvider;
using EventHandlerSharedPtr = std::shared_ptr<EventHandlingProvider>;
using EventHandlerWeakPtr = std::weak_ptr<EventHandlingProvider>;

class RunLoop;
using RunLoopSharedPtr = std::shared_ptr<RunLoop>;
using RunLoopWeakPtr = std::weak_ptr<RunLoop>;

class SocketNotifier;
using SocketNotifierSharedPtr = std::shared_ptr<SocketNotifier>;
using SocketNotifierWeakPtr = std::weak_ptr<SocketNotifier>;

class AbstractRunLoopSource;
using AbstractRunLoopSourceSharedPtr = std::shared_ptr<AbstractRunLoopSource>;

class TimerSource;
using TimerSourcePtr = std::shared_ptr<TimerSource>;
using TimerSourceWeakPtr = std::weak_ptr<TimerSource>;

class EventSource;
using EventSourcePtr = std::shared_ptr<EventSource>;

class SocketNotifierSource;
using SocketNotifierSourcePtr = std::shared_ptr<SocketNotifierSource>;
using SocketNotifierSourceWeakPtr = std::weak_ptr<SocketNotifierSource>;

/// The event processing flags.
enum class ProcessFlags
{
    /// Process all events.
    ProcessAll = 0xFF
};
ENABLE_ENUM_OPERATORS(ProcessFlags);

}

DECLARE_LOG_CATEGORY(event)

#endif // EVENT_HANDLING_DECLARATIONS_HPP
