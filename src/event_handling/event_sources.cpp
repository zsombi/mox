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

#include <mox/event_handling/event_sources.hpp>
#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/event_loop.hpp>
#include <mox/event_handling/event_handler.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/object.hpp>
#include <mox/timer.hpp>

namespace mox
{

/******************************************************************************
 *
 */
AbstractEventSource::AbstractEventSource(std::string_view name)
    : m_name(name)
{
}

void AbstractEventSource::setEventDispatcher(EventDispatcher& eventDispatcher)
{
    m_eventDispatcher = eventDispatcher.shared_from_this();
}

std::string AbstractEventSource::name() const
{
    return std::string(m_name);
}

EventDispatcherSharedPtr AbstractEventSource::eventDispatcher() const
{
    return m_eventDispatcher.lock();
}

void AbstractEventSource::prepare()
{
}

void AbstractEventSource::shutDown()
{
}

/******************************************************************************
 * TimerSource
 */
TimerSource::TimerSource(std::string_view name)
    : AbstractEventSource(name)
{
}
void TimerSource::signal(Timer& timer)
{
    timer.expired(&timer);

    if (timer.type() == Timer::Type::SingleShot)
    {
        timer.stop();
    }
}

/******************************************************************************
 * PostEventSource
 */
PostEventSource::PostEventSource(std::string_view name)
    : AbstractEventSource(name)
    , m_eventQueue(ThreadData::thisThreadData()->m_eventQueue)
{
}

void PostEventSource::dispatch()
{
    FATAL(eventDispatcher(), "Orphan event source?")
    if (!eventDispatcher()->isProcessingEvents())
    {
        std::cerr << "Warning: source attempting processing posted events when the event loop is not running." << std::endl;
        return;
    }
    m_eventQueue.process(sendEvent);
}

/******************************************************************************
 * SocketNotifierSource
 */
SocketNotifierSource::SocketNotifierSource(std::string_view name)
    : AbstractEventSource(name)
{
}

}
