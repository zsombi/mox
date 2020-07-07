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
#include <mox/core/event_handling/event_queue.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/object.hpp>
#include <mox/core/timer.hpp>
#include <mox/core/platforms/adaptation.hpp>

#include <utility>

namespace mox
{

/******************************************************************************
 * RunLoopBase
 */
void RunLoopBase::notifyRunLoopDown()
{
    m_status = Status::Stopped;
    if (m_closedCallback)
    {
        m_closedCallback();
    }
}

void RunLoopBase::setEventProcessingCallback(EventProcessingCallback callback)
{
    m_processEventsCallback = callback;
}

void RunLoopBase::setRunLoopDownCallback(DownCallback callback)
{
    m_closedCallback = callback;
}

bool RunLoopBase::startTimer(TimerCore& timer)
{
    if (m_status >= Status::Exiting)
    {
        return false;
    }
    startTimerOverride(timer);
    return true;
}

void RunLoopBase::removeTimer(TimerCore& timer)
{
    removeTimerOverride(timer);
}

bool RunLoopBase::attachSocketNotifier(SocketNotifierCore& notifier)
{
    if (m_status >= Status::Exiting)
    {
        return false;
    }
    attachSocketNotifierOverride(notifier);
    return true;
}

void RunLoopBase::detachSocketNotifier(SocketNotifierCore& notifier)
{
    detachSocketNotifierOverride(notifier);
}

void RunLoopBase::scheduleSources()
{
    scheduleSourcesOverride();
}

void RunLoopBase::quit()
{
    if (m_status == Status::Exiting)
    {
        CTRACE(event, "The runloop is already exiting.");
        return;
    }

    stopRunLoop();

    m_status = Status::Exiting;
}

RunLoopBase::Status RunLoopBase::getStatus() const
{
    return m_status;
}

void RunLoopBase::onIdle(IdleFunction&& idle)
{
    if (m_status > Status::Running)
    {
        return;
    }
    onIdleOverride(std::forward<IdleFunction>(idle));
}

/******************************************************************************
 * RunLoop
 */
RunLoop::~RunLoop()
{
    CTRACE(event, "RunLoop died");
}

RunLoopPtr RunLoop::create(bool main)
{
    auto evLoop = Adaptation::createRunLoop(main);

    return evLoop;
}

/******************************************************************************
 * RunLoopHook
 */

RunLoopHookPtr RunLoopHook::create()
{
    auto evLoop = Adaptation::createRunLoopHook();

    return evLoop;
}

}
