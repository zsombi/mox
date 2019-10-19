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

#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

#include <mox/utils/globals.hpp>
#include <mox/event_handling/event.hpp>

#include <functional>
#include <list>
#include <unordered_map>

namespace mox
{

class Event;

/// This is the base class for event handlers.
///
/// Events are dispatched to the event handlers in two phases: tunneling and bubbling.
/// In each phase the event is dispatched to objects that fall in between the root
/// object and the target object of the event.
///
/// During tunneling phase, the event is filtered out from being handled. This is realized
/// by dispatching the event to the event handler objects from root to the target node.
/// The event is marked as handled before it is passed to the filterEvent() method. If
/// the method returns \e true, the event is filtered out, and the event dispatching ends.
/// If the methid returns false, the event is unmarked from being handled and the event
/// dispatching continues, till the target object is reached, or an ascendant event handler
/// filters out the message.
///
/// During bubbling phase the event is handed to the processEvent() method. The event is
/// marked as handled before it is handed over to the method. If the event is consumed, the
/// event handler can leave the event marked as handled. If the event is not consumed, the
/// handler must mark the event as unhandled. In this case the event is bubbled to the
/// closest ascendant event handler. The bubbling is repeated until an event handler
/// consumes the event.
class MOX_API EventHandlingProvider
{
public:
    using EventFilter = std::function<bool(Event&)>;
    using EventHandler = std::function<void(Event&)>;

    class MOX_API Token
    {
    public:
        virtual ~Token() = default;

    protected:
        explicit Token() = default;
    };
    using TokenPtr = std::shared_ptr<Token>;

    /// Destructor.
    virtual ~EventHandlingProvider();

    TokenPtr addEventHandler(EventType type, const EventHandler& handler);
    TokenPtr addEventFilter(EventType type, const EventFilter& filter);

protected:
    /// Constructor;
    explicit EventHandlingProvider() = default;
    /// Method called on filtering phase, when an event is dispatched to an object.
    /// \return If the event is filtered out, returns \e true, otherwise \e false.
    bool filterEvent(Event& event);
    /// Method called after filtering phase, to handle the event.
    void processEvent(Event& event);

    friend class EventLoop;

private:
    using TokenList = std::list<TokenPtr>;
    using Container = std::unordered_map<EventType, TokenList>;

    Container m_handlers;
    Container m_filters;
};

}

#endif // EVENT_HANDLER_HPP
