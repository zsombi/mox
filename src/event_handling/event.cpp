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
#include <mox/object.hpp>

namespace mox
{

/******************************************************************************
 * Event
 */
Event::Event(EventType type, ObjectSharedPtr target, Priority priority)
    : m_target(target)
    , m_type(type)
    , m_priority(priority)
{
    FATAL(target, "Event created without a valid target")
}

ObjectSharedPtr Event::target() const
{
    return m_target.lock();
}

EventType Event::type() const
{
    return m_type;
}

Event::Priority Event::priority() const
{
    return m_priority;
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

EventType Event::registerNewType()
{
    static EventType userType = EventType::UserType;
    return ++userType;
}

/******************************************************************************
 * QuitEvent
 */
QuitEvent::QuitEvent(ObjectSharedPtr target, int exitCode)
    : Event(EventType::Quit, target, Priority::Normal)
    , m_exitCode(exitCode)
{
}

int QuitEvent::getExitCode() const
{
    return m_exitCode;
}

/******************************************************************************
 * DeferredSignalEvent
 */
DeferredSignalEvent::DeferredSignalEvent(Object& target, Signal::Connection& connection, const Callable::ArgumentPack& args)
    : Event(EventType::DeferredSignal, target.shared_from_this(), Priority::Urgent)
    , m_connection(connection.shared_from_this())
    , m_arguments(args)
{
}

void DeferredSignalEvent::activate()
{
    TRACE("Asynchronously activate connection for target " << target())
    if (target() && m_connection && m_connection->isConnected())
    {
        TRACE("Activate...");
        m_connection->activate(m_arguments);
        TRACE("Activation completed");
    }
}

} // mox
