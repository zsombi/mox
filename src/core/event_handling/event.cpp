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
Event::Event(ObjectSharedPtr target, EventType type, Priority priority)
    : m_target(target)
    , m_type(type)
    , m_priority(priority)
{
    FATAL(target, "Event created without a valid target");
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

bool Event::isCompressible() const
{
    return true;
}

bool Event::canCompress(const Event& other)
{
    return (m_type == other.m_type) && (m_target.lock() == other.m_target.lock());
}

/******************************************************************************
 * QuitEvent
 */
QuitEvent::QuitEvent(ObjectSharedPtr target, int exitCode)
    : Event(target, EventType::Quit, Priority::Normal)
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
DeferredSignalEvent::DeferredSignalEvent(ObjectSharedPtr target, Signal::Connection& connection, const Callable::ArgumentPack& args)
    : Event(target, EventType::DeferredSignal, Priority::Urgent)
    , m_connection(connection.shared_from_this())
    , m_arguments(args)
{
    FATAL(target, "Cannot post deferred call on a null target");
}

void DeferredSignalEvent::activate()
{
    CTRACE(event, "Asynchronously activate connection for target" << target());
    if (target() && m_connection && m_connection->isConnected())
    {
        CTRACE(event, "Activate...");
        m_connection->activate(m_arguments);
        CTRACE(event, "Activation completed");
    }
}

} // mox
