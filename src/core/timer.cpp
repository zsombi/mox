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

#include <mox/core/timer.hpp>
#include <mox/core/process/thread_data.hpp>
#include <process_p.hpp>

namespace mox
{

Timer::Timer(Type type, std::chrono::milliseconds interval)
    : TimerSource::TimerRecord(interval, type == Type::SingleShot)
{
}

TimerPtr Timer::createSingleShot(std::chrono::milliseconds timeout)
{
    TimerPtr timer(new Timer(Type::SingleShot, timeout));
    return timer;
}

TimerPtr Timer::createRepeating(std::chrono::milliseconds interval)
{
    TimerPtr timer(new Timer(Type::Repeating, interval));
    return timer;
}

void Timer::start()
{
    stop();
    auto thread = ThreadData::getThisThreadData()->thread();
    auto d = ThreadInterfacePrivate::get(*thread);
    auto source = d->runLoop->getDefaultTimerSource();
    TimerSource::TimerRecord::start(*source);
}

void Timer::signal()
{
    expired(this);
    if (isSingleShot() && isRunning())
    {
        stop();
    }
}

}
