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

#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include <mox/utils/globals.hpp>
#include <mox/event_handling/event_queue.hpp>
#include <mox/event_handling/event_dispatcher.hpp>

namespace mox
{

class ThreadData;

/// The EventLoop class provides the event handling in Mox. Each event loop is subscribed
/// to the current thread's event dispatcher. The event dispatcher is assumed to contain
/// a post-event source, which schedules the event dispatching. During the event dispatch,
/// the events accummulated in the event queue of the loop are processed and sent to their
/// targets.
///
/// You can have multiple event loops on a thread, but only one of those can be active.
///
/// An event loop is activated by declaring the event loop on the stack. Once declared on
/// the stack, you can call the processEvents() method to process the events on the loop.
/// The event loop is deactivated only when it gets destroyed.
class MOX_API EventLoop
{
public:
    /// Constructor, sets this event loop as the active event loop in the current event
    /// dispatcher.
    explicit EventLoop();
    /// Destructor, removes the event loop from the event dispatcher.
    ~EventLoop();

    /// Invokes event processing of a local event loop.
    int processEvents(ProcessFlags flags = ProcessFlags::ProcessAll);

    /// Exit the current event loop.
    void exit(int exitCode = 0);

    /// Wakes up the event loop.
    void wakeUp();

private:
    friend class PostEventSource;

    ThreadData& m_threadData;
    EventDispatcherSharedPtr m_dispatcher;
};

/// Posts an \a event to handle asynchronously.
/// \return If the thread the event's target object belongs has no event dispatcher, or event loop, returns \e false, otherwise \e true.
bool MOX_API postEvent(EventPtr&& event);

/// Sends the event to the target object specified in the event. Returns true
bool MOX_API sendEvent(Event& event);

template <class EventClass, typename... Arguments>
bool postEvent(Arguments&&... arguments)
{
    return postEvent(make_event<EventClass>(std::forward<Arguments>(arguments)...));
}

}

#endif // EVENT_LOOP_HPP
