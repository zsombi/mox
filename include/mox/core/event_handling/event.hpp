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

#include <mox/config/deftypes.hpp>
#include <mox/core/meta/argument_data.hpp>
#include <mox/core/meta/signals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/type_traits.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>

#include <chrono>

namespace mox
{

class Object;
using ObjectSharedPtr = std::shared_ptr<Object>;
using ObjectWeakPtr = std::weak_ptr<Object>;

/// The event type identifier.
enum class EventId : int32_t
{
    Base,
    Quit,
    UserType = 100
};
ENABLE_ENUM_OPERATORS(EventId)

/// Specifies the event priority.
enum class EventPriority : uint32_t
{
    Urgent = 0,
    Normal = 1000,
    Low = 5000
};
ENABLE_ENUM_OPERATORS(EventPriority);

/// The event identifier.
using EventType = std::pair<EventId, EventPriority>;

constexpr EventType BaseEvent = {EventId::Base, EventPriority::Normal};
constexpr EventType QuitEvent = {EventId::Quit, EventPriority::Urgent};

/// This is the base class for all Mox events. The events are composed of a type, priority
/// and a handler. The handler is the object which receives the event.
class MOX_API Event
{

public:    
    /// Constructs an event with a \a type and a \a target.
    explicit Event(ObjectSharedPtr target, EventType type);
    /// Descructor.
    virtual ~Event() = default;

    /// Returns the target of the event.
    ObjectSharedPtr target() const;
    /// Returns the type of the event.
    EventId type() const;
    /// Returns the priority of the event.
    EventPriority priority() const;
    /// Returns \e true if the event is handled, \e false otherwise.
    bool isHandled() const;
    /// Sets the handled state of an event.
    void setHandled(bool handled = true);

    /// Marks the time the method is called as timestamp for the event.
    /// EventQueue uses this when the event is pushed to the queue.
    void markTimestamp();
    /// Returns the timestamp of the event.
    Timestamp timestamp() const;

    /// \name Event compression
    /// Event compression is applied right before the event is pushed into an event queue. If an event
    /// supports compression, the event is tested agains the queued events to see if the event compression
    /// applies. The event type itself decides the clauses where event compression is applied. If the
    /// event is tested positive on compression, the event will not land in the event queue.
    /// \{
    ///
    /// Returns the event compression supported state.
    /// \return If the event is compressible, returns \e true, otherwise \e false. The default implementation
    /// returns \e true.
    virtual bool isCompressible() const;

    /// Called by the EventQueue when an Event::isCompressible() returns \e true on the event pushed
    /// to the queue. Checks whether this event is compressable with the \a other.
    /// \param other The other event to test.
    /// \return If this event compresses into the \a other, returns \e true, otherwise \e false. The default
    /// implementation takes the event type and the event target as criterias for compressibility.
    virtual bool canCompress(const Event& other);
    /// \}

    /// Registers a new event type. Returns the newly registered event type.
    static EventType registerNewType(EventPriority priority = EventPriority::Normal);

private:
    DISABLE_COPY(Event)

    ObjectWeakPtr m_target;
    Timestamp m_timeStamp;
    EventType m_id = {EventId::Base, EventPriority::Normal};
    bool m_isHandled = false;
};

class MOX_API QuitEventType : public Event
{
    DISABLE_COPY(QuitEventType)
    int m_exitCode = 0;

public:
    explicit QuitEventType(ObjectSharedPtr target, int exitCode = 0);

    int getExitCode() const;
};

template <class EventClass, class TargetPtr, typename... Arguments>
auto make_event(TargetPtr target, Arguments&&... arguments)
{
    static_assert(is_shared_ptr<TargetPtr>::value, "The first argument must be a shared pointer to the target");
    return std::make_unique<EventClass>(target, std::forward<Arguments>(arguments)...);
}

} // namespace mox

#endif // EVENT_HPP
