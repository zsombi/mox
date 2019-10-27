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

#ifndef EVENT_HPP
#define EVENT_HPP

#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <mox/signal/signal.hpp>

#include <chrono>

namespace mox
{

class Object;
using ObjectSharedPtr = std::shared_ptr<Object>;
using ObjectWeakPtr = std::weak_ptr<Object>;

/// The event type identifier.
enum class EventType : int32_t
{
    Base,
    DeferredSignal,
    UserType = 100
};
ENABLE_ENUM_OPERATORS(EventType)

/// This is the base class for all Mox events. The events are composed of a type, priority
/// and a handler. The handler is the object which receives the event.
class MOX_API Event
{

public:
    /// Specifies the event priority.
    enum class Priority : uint32_t
    {
        Urgent = 0,
        Normal = 1000,
        Low = 5000
    };

    /// Constructs an event with a \a type, a \a target and \a priority.
    explicit Event(EventType type, ObjectSharedPtr target, Priority priority = Priority::Normal);
    /// Descructor.
    virtual ~Event() = default;

    /// Returns the target of the event.
    ObjectSharedPtr target() const;
    /// Returns the type of the event.
    EventType type() const;
    /// Returns the priority of the event.
    Priority priority() const;
    /// Returns \e true if the event is handled, \e false otherwise.
    bool isHandled() const;
    /// Sets the handled state of an event.
    void setHandled(bool handled = true);

    /// Marks the time the method is called as timestamp for the event.
    /// EventQueue uses this when the event is pushed to the queue.
    void markTimestamp();
    /// Returns the timestamp of the event.
    Timestamp timestamp() const;


    /// Registers a new event type. Returns the newly registered event type.
    static EventType registerNewType();

private:
    DISABLE_COPY(Event)

    ObjectWeakPtr m_target;
    Timestamp m_timeStamp;
    EventType m_type = EventType::Base;
    Priority m_priority = Priority::Normal;
    bool m_isHandled = false;
};

class MOX_API DeferredSignalEvent : public Event
{
    Signal::ConnectionSharedPtr m_connection;
    Callable::ArgumentPack m_arguments;
public:
    explicit DeferredSignalEvent(ObjectSharedPtr target, Signal::ConnectionSharedPtr connection, const Callable::ArgumentPack& args);

    void activate();
};

} // namespace mox

#endif // EVENT_HPP
