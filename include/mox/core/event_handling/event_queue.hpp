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
#ifndef EVENT_QUEUE_HPP
#define EVENT_QUEUE_HPP

#include <mox/utils/locks.hpp>
#include <mox/core/event_handling/event_handling_declarations.hpp>
#include <mox/core/event_handling/event.hpp>
#include <mox/utils/log/logger.hpp>

#include <functional>
#include <queue>

namespace mox
{

struct MOX_API EventQueueComparator
{
    bool operator()(const EventPtr& lhs, const EventPtr& rhs) const;
};

using EventQueueBase = std::priority_queue<EventPtr, std::vector<EventPtr>, EventQueueComparator>;

/// EventQueue implements the prioritized queueing of events to handle.
class MOX_API EventQueue : public Lockable, protected EventQueueBase
{
public:
    /// Constructor.
    explicit EventQueue() = default;

    /// Descructor.
    ~EventQueue() = default;

    /// Clears the event queue.
    void clear();
    /// Returns the size of the event queue. The method is not thread-safe.
    size_t size() const;
    /// Returns \e true if the event queue is empty, \e false otherwise. The method is not thread-safe.
    bool empty() const;

    /// Pushes an \a event to the event queue. Updates the timestamp of the event pushed.
    void push(EventPtr event);
    /// Processes the event queue, popping each event from the queue and passing those
    /// to the \a dispatcher function. The processing continues till there are events in
    /// the queue and till the dispatcher returns true. When the dispatcher returns false,
    /// the queue processing is stopped and the remaining events are discarded from the
    /// event queue.
    /// The function always returns with an empty event queue.
    template <typename DispatchFunction>
    void dispatch(DispatchFunction dispatcher)
    {
        lock_guard lock(*this);
        while (!empty())
        {
            EventPtr ev(std::move(c.front()));
            pop();

            CTRACE(event, "Processing event:" << int(ev->type()));
            ScopeRelock relock(*this);
            dispatcher(*ev);
        }
    }
};

}

#endif // EVENT_QUEUE_HPP
