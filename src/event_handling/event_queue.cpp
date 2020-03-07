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

#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_queue.hpp>


namespace mox
{

bool EventQueueComparator::operator()(const EventPtr& lhs, const EventPtr& rhs) const
{
    if (lhs->priority() == rhs->priority())
    {
        // Fall back on timestamp.
        return lhs->timestamp() > rhs->timestamp();
    }
    return lhs->priority() > rhs->priority();
}

EventQueue::~EventQueue()
{
}

void EventQueue::clear()
{
    lock_guard lock(*this);
    // Access the container to wipe the queue.
    c.clear();
}

size_t EventQueue::size() const
{
    return EventQueueBase::size();
}

bool EventQueue::empty() const
{
    return EventQueueBase::empty();
}

void EventQueue::push(EventPtr event)
{
    lock_guard lock(*this);
    if (event->isCompressible())
    {
        // loop through the container and find out if compression is needed
        auto testCompression = [&event](const EventPtr& qEvent)
        {
            return event->canCompress(*qEvent);
        };
        auto compress = std::find_if(c.rbegin(), c.rend(), testCompression);
        if (compress != c.rend())
        {
            // Compression required, so bail out.
            return;
        }
    }

    // No compression is required, proceed with push.
    event->markTimestamp();
    EventQueueBase::emplace(std::move(event));
}

}
