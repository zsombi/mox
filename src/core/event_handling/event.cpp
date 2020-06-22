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

#include <mox/core/event_handling/event.hpp>
#include <mox/core/object.hpp>

namespace mox
{

/******************************************************************************
 * Event
 */
Event::Event(ObjectSharedPtr target, EventType type)
    : m_target(target)
    , m_id(type)
{
    FATAL(target, "Event created without a valid target");
}

ObjectSharedPtr Event::target() const
{
    return m_target.lock();
}

EventId Event::type() const
{
    return m_id.first;
}

EventPriority Event::priority() const
{
    return m_id.second;
}

bool Event::isHandled() const
{
    return m_isHandled;
}

void Event::setHandled(bool handled)
{
    m_isHandled = handled;
}

void Event::markTimestamp()
{
    m_timeStamp = std::chrono::system_clock::now();
}

Timestamp Event::timestamp() const
{
    return m_timeStamp;
}

EventType Event::registerNewType(EventPriority priority)
{
    static EventId userType = EventId::UserType;
    return std::make_pair(++userType, priority);
}

bool Event::isCompressible() const
{
    return true;
}

bool Event::canCompress(const Event& other)
{
    return (m_id.first == other.m_id.first) && (m_target.lock() == other.m_target.lock());
}

/******************************************************************************
 * QuitEvent
 */
QuitEventType::QuitEventType(ObjectSharedPtr target, int exitCode)
    : Event(target, QuitEvent)
    , m_exitCode(exitCode)
{
}

int QuitEventType::getExitCode() const
{
    return m_exitCode;
}

} // mox
