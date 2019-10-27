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

namespace mox
{

class Event;
using EventPtr = std::unique_ptr<Event>;

class EventQueue;
using EventQueueSharedPtr = std::shared_ptr<EventQueue>;

class EventHandlingProvider;
using EventHandlerSharedPtr = std::shared_ptr<EventHandlingProvider>;
using EventHandlerWeakPtr = std::weak_ptr<EventHandlingProvider>;

class EventDispatcher;
using EventDispatcherSharedPtr = std::shared_ptr<EventDispatcher>;
using EventDispatcherWeakPtr = std::weak_ptr<EventDispatcher>;

class SocketNotifier;
using SocketNotifierSharedPtr = std::shared_ptr<SocketNotifier>;
using SocketNotifierWeakPtr = std::weak_ptr<SocketNotifier>;

class AbstractEventSource;
using AbstractEventSourceSharedPtr = std::shared_ptr<AbstractEventSource>;

class TimerSource;
using TimerSourcePtr = std::shared_ptr<TimerSource>;
using TimerSourceWeakPtr = std::weak_ptr<TimerSource>;

class PostEventSource;
using PostEventSourcePtr = std::shared_ptr<PostEventSource>;

class SocketNotifierSource;
using SocketNotifierSourcePtr = std::shared_ptr<SocketNotifierSource>;
using SocketNotifierSourceWeakPtr = std::weak_ptr<SocketNotifierSource>;

class EventLoop;
using EventLoopPtr = EventLoop*;

/// Defines the state of the event processing.
enum class EventDispatchState
{
    /// The dispatcher is inactive.
    Inactive,
    /// The dispatcher is running.
    Running,
    /// The dispatcher is suspended. Not all platforms support this state.
    Suspended,
    /// The dispatcher is exiting.
    Exiting,
    /// The dispatcher is stopped.
    Stopped
};

/// The event processing flags.
enum class ProcessFlags
{
    /// Run the event processing one loop cycle, till the first time the idle is reached.
    RunOnce = 0x01,
    ProcessAll = 0xFF
};
ENABLE_ENUM_OPERATORS(ProcessFlags);

class no_event_dispatcher : public std::exception
{
public:
    const char* what() const EXCEPTION_NOEXCEPT override
    {
        return "No EventDispatcher is set on the thread";
    }
};

class no_event_loop : public std::exception
{
public:
    const char* what() const EXCEPTION_NOEXCEPT override
    {
        return "No EventLoop is set on the thread to handle posted events";
    }
};

}

#endif // EVENT_HANDLING_DECLARATIONS_HPP
