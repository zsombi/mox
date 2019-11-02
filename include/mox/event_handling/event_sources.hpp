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

#ifndef EVENT_SOURCES_HPP
#define EVENT_SOURCES_HPP

#include <mox/utils/globals.hpp>
#include <mox/event_handling/event_handling_declarations.hpp>
#include <mox/event_handling/event_queue.hpp>

namespace mox
{

class Timer;

/// Base class for event sources.
class MOX_API AbstractEventSource : public std::enable_shared_from_this<AbstractEventSource>
{
public:
    /// Destructor.
    virtual ~AbstractEventSource() = default;
    /// Returns the name of the event source.
    std::string name() const;
    /// Returns the event dispatcher.
    EventDispatcherSharedPtr eventDispatcher() const;

    /// Set the event dispatcher to which this event source is attached.
    virtual void setEventDispatcher(EventDispatcher& eventDispatcher);

    /// The method is called when the event dispatcher event processing starts.
    virtual void prepare();

    /// The method is called when the event dispatcher's event processing is exiting.
    virtual void shutDown();

protected:
    /// Constructor.
    explicit AbstractEventSource(std::string_view name);

    EventDispatcherWeakPtr m_eventDispatcher;
    std::string m_name;
};

/// The TimerSource provides support for single shot and periodic timers in Mox.
/// An event dispatcher can have many timer event sources.
class MOX_API TimerSource : public AbstractEventSource
{
public:
    /// Adds a timer object to the source.
    virtual void addTimer(Timer& timer) = 0;
    /// Removes a timer object from the source.
    virtual void removeTimer(Timer& timer) = 0;
    /// Returns the running timer count in the source,
    virtual size_t timerCount() const = 0;

    /// Signals the timer passed as argument.
    void signal(Timer& timer);

protected:
    /// Constructs the event source.
    explicit TimerSource(std::string_view name);
};

/// This class defines the interface for the post-event sources. Each post event source has
/// an event queue of its own.
class MOX_API PostEventSource : public AbstractEventSource
{
public:
    /// Notifies the event soure to re-schedule.
    virtual void wakeUp() = 0;

    /// Calls into event dispatcher to invoke the event loop dispatching.
    void dispatch();

protected:
    /// Constructor.
    explicit PostEventSource(std::string_view name);
    /// The event queue to process.
    EventQueue& m_eventQueue;
};

/// This class defines the interface for the socket notifier event sources.
class MOX_API SocketNotifierSource : public AbstractEventSource
{
public:
    /// Add a socket notifier to the event source.
    virtual void addNotifier(SocketNotifier& notifier) = 0;
    /// Remove a socket notifier from the event source.
    virtual void removeNotifier(SocketNotifier& notifier) = 0;

protected:
    explicit SocketNotifierSource(std::string_view name);
};

}

#endif // EVENT_SOURCES_HPP
