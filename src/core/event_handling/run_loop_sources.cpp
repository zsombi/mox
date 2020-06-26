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

#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/event_handling/event.hpp>
#include <mox/core/platforms/adaptation.hpp>
#include <mox/core/object.hpp>
#include <mox/core/timer.hpp>

namespace mox
{

/******************************************************************************
 * TimerCore
 */
static int32_t timerUId = 0;

TimerCore::TimerCore(std::chrono::milliseconds interval, bool singleShot)
    : m_interval(interval)
    , m_id(++timerUId)
    , m_singleShot(singleShot)
    , m_isRunning(true)
{
}

TimerCore::~TimerCore()
{
    stop();
}

void TimerCore::start(RunLoopBase& runLoop)
{
    m_runLoop = runLoop.shared_from_this();
    runLoop.startTimer(*this);
}

void TimerCore::stop()
{
    auto loop = m_runLoop.lock();
    if (loop)
    {
        loop->removeTimer(*this);
        m_runLoop.reset();
    }
}

/******************************************************************************
 * SocketNotifierCore
 */
SocketNotifierCore::SocketNotifierCore(EventTarget handler, Modes modes)
    : m_handler(handler)
    , m_modes(modes & Adaptation::supportedModes())
{
}

SocketNotifierCore::~SocketNotifierCore()
{
    detach();
}

void SocketNotifierCore::attach(RunLoopBase& runLoop)
{
    if (runLoop.attachSocketNotifier(*this))
    {
        m_runLoop = runLoop.shared_from_this();
    }
}

void SocketNotifierCore::detach()
{
    auto loop = m_runLoop.lock();
    if (!loop)
    {
        return;
    }
    loop->detachSocketNotifier(*this);
    m_runLoop.reset();
}

}
