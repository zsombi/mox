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

#include <mox/timer.hpp>
#include <mox/module/thread_data.hpp>

namespace mox
{

static int32_t timerUId = 0;

Timer::Timer(Type type, std::chrono::milliseconds interval)
    : m_source(std::dynamic_pointer_cast<TimerSource>(ThreadData::thisThreadData()->eventDispatcher()->findEventSource("default_timer")))
    , m_interval(interval)
    , m_type(type)
    , m_id(++timerUId)
{
}

Timer::~Timer()
{
    stop();
}

TimerPtr Timer::createSingleShot(std::chrono::milliseconds timeout)
{
    TimerPtr timer(new Timer(Type::SingleShot, timeout));
    return timer;
}

TimerPtr Timer::createRepeating(std::chrono::milliseconds timeout)
{
    TimerPtr timer(new Timer(Type::Repeating, timeout));
    return timer;
}

void Timer::start()
{
    stop();
    TimerSourcePtr source = m_source.lock();
    if (!m_isRunning && source)
    {
        source->addTimer(*this);
        m_isRunning = true;
    }
}

void Timer::stop()
{
    TimerSourcePtr source = m_source.lock();
    if (m_isRunning && source)
    {
        m_isRunning = false;
        source->removeTimer(*this);
    }
}

}
