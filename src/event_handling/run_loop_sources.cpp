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

#include <mox/event_handling/run_loop.hpp>
#include <mox/event_handling/run_loop_sources.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/object.hpp>
#include <mox/timer.hpp>

namespace mox
{

/******************************************************************************
 *
 */
AbstractRunLoopSource::AbstractRunLoopSource(std::string_view name)
    : m_name(name)
{
}

void AbstractRunLoopSource::setRunLoop(RunLoop& runLoop)
{
    m_runLoop = runLoop.shared_from_this();
}

std::string AbstractRunLoopSource::name() const
{
    return std::string(m_name);
}

RunLoopSharedPtr AbstractRunLoopSource::getRunLoop() const
{
    return m_runLoop.lock();
}

void AbstractRunLoopSource::prepare()
{
}

void AbstractRunLoopSource::clean()
{
}

/******************************************************************************
 * TimerSource
 */

static int32_t timerUId = 0;

TimerSource::TimerRecord::TimerRecord(std::chrono::milliseconds interval, bool singleShot)
    : m_interval(interval)
    , m_id(++timerUId)
    , m_singleShot(singleShot)
    , m_isRunning(true)
{
}

TimerSource::TimerRecord::~TimerRecord()
{
    stop();
}

void TimerSource::TimerRecord::start(TimerSource& timerSource)
{
    FATAL(!m_source || m_source.get() == &timerSource, "Invalid or different source applied");
    m_source = std::static_pointer_cast<TimerSource>(timerSource.shared_from_this());
    m_source->addTimer(*this);
}

void TimerSource::TimerRecord::stop()
{
    if (m_source)
    {
        auto source = m_source;
        m_source.reset();
        source->removeTimer(*this);
    }
}

TimerSource::TimerSource(std::string_view name)
    : AbstractRunLoopSource(name)
{
}

/******************************************************************************
 * EventSource
 */
EventSource::EventSource(std::string_view name)
    : AbstractRunLoopSource(name)
{
}

void EventSource::dispatchQueuedEvents()
{
    FATAL(getRunLoop(), "Orphan event source?");
    if (!getRunLoop()->isRunning())
    {
        WARN("source attempting processing posted events when the event loop is not running.");
        return;
    }

    auto dispatchEvent = [](auto& event)
    {
        auto dispatcher = std::static_pointer_cast<EventDispatcher>(event.target());

        if (!dispatcher)
        {
            return;
        }

        dispatcher->dispatchEvent(event);
    };

    m_eventQueue.process(dispatchEvent);
}

void EventSource::push(EventPtr event)
{
    m_eventQueue.push(std::move(event));
    getRunLoop()->wakeUp();
}

/******************************************************************************
 * SocketNotifierSource
 */
SocketNotifierSource::Notifier::Notifier(EventTarget handler, Modes modes)
    : m_handler(handler)
    , m_modes(modes & SocketNotifierSource::supportedModes())
{
}

SocketNotifierSource::Notifier::~Notifier()
{
    detach();
}

void SocketNotifierSource::Notifier::attach(SocketNotifierSource& source)
{
    m_source = std::static_pointer_cast<SocketNotifierSource>(source.shared_from_this());
    source.addNotifier(*this);
}

void SocketNotifierSource::Notifier::detach()
{
    auto source = m_source.lock();
    if (!source)
    {
        return;
    }
    source->removeNotifier(*this);
    m_source.reset();
}

SocketNotifierSource::SocketNotifierSource(std::string_view name)
    : AbstractRunLoopSource(name)
{
}

}
