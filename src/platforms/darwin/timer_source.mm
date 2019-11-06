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

#include "event_dispatcher.h"

namespace mox
{

CFTimerSource::TimerRecord::TimerRecord(Timer& handler)
    : timerHandler(handler.shared_from_this())
{
}

CFTimerSource::TimerRecord::~TimerRecord()
{
    if (timerRef)
    {
        TRACE("Deleting timer: " << timerRef)
        CFRunLoopTimerInvalidate(timerRef);
        CFRelease(timerRef);
        timerRef = nullptr;
    }
    timerHandler.reset();
}

void CFTimerSource::TimerRecord::create(CFTimerSource& source)
{
    TRACE("Create timer record for source")
    if (!timerHandler || (timerRef && CFRunLoopTimerIsValid(timerRef)))
    {
        return;
    }
    if (timerRef)
    {
        TRACE("WARNING: recreating timer?!")
        CFRelease(timerRef);
    }
    CFAbsoluteTime timeout = CFAbsoluteTime(timerHandler->interval().count()) / 1000;
    CFAbsoluteTime timeToFire = CFAbsoluteTimeGetCurrent() + timeout;
    CFAbsoluteTime interval = timerHandler->type() == Timer::Type::SingleShot ? -1 : timeout;

    auto proc = [&source, self = this](CFRunLoopTimerRef)
    {
        if (!self->timerHandler)
        {
            return;
        }

        lock_guard lock(source.timers);
        TRACE("Signaling timer " << self->timerRef << " of self[" << self << ']' << source.timers.lockCount())
        TimerPtr keepAlive = self->timerHandler;
        source.signal(*keepAlive);
        TRACE("Timer signaled: " << self->timerRef << " of self[" << self << ']' << source.timers.lockCount())
    };
    timerRef = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, timeToFire, interval, 0, 0, proc);
    CFEventDispatcher* eventDispatcher = static_cast<CFEventDispatcher*>(source.eventDispatcher().get());
    CFRunLoopAddTimer(eventDispatcher->runLoop, timerRef, kCFRunLoopCommonModes);
    FATAL(CFRunLoopTimerIsValid(timerRef), "Invalid timer created!")
    TRACE("Timer created with interval: " << interval << ", timeout " << timeout << ", ref: " << timerRef)
}

/******************************************************************************
 * CFEventDispatcher
 */
size_t CFEventDispatcher::runningTimerCount() const
{
    size_t count = 0;
    auto counter = [&count](TimerSourcePtr source)
    {
        count += source->timerCount();
    };
    forEachSource<TimerSource>(counter);
    return count;
}

/******************************************************************************
 * TimerSource
 */

void CFTimerSource::addTimer(Timer& timer)
{
    auto predicate = [&timer](TimerRecordPtr& record)
    {
        return record->timerHandler.get() == &timer;
    };
    auto index = timers.findIf(predicate);
    FATAL(!index, "Timer already registered")

    lock_guard lock(timers);
    timers.emplace(std::make_unique<TimerRecord>(timer));
}

void CFTimerSource::removeTimer(Timer &timer)
{
    auto predicate = [&timer](TimerRecordPtr& record)
    {
        return record->timerHandler.get() == &timer;
    };
    auto index = timers.findIf(predicate);
    FATAL(index, "Timer not registered")

    lock_guard lock(timers);
    timers[*index]->timerHandler.reset();
}

void CFTimerSource::shutDown()
{
    lock_guard lock(timers);
    auto looper = [](TimerRecordPtr& timer)
    {
        if (timer->timerHandler)
        {
            timer->timerHandler->stop();
        }
    };
    timers.forEach(looper);
}

size_t CFTimerSource::timerCount() const
{
    return timers.size();
}

void CFTimerSource::activate()
{
    auto loop = [self = this](TimerRecordPtr& timer)
    {
        if (!timer->timerHandler)
        {
            return;
        }

        if (timer->timerHandler->type() == Timer::Type::Repeating)
        {
            // Recreate timeRef if invalid.
            if (!timer->timerRef || !CFRunLoopTimerIsValid(timer->timerRef))
            {
                TRACE("Recreate repeating timer")
                timer->create(*self);
            }
        }

        if (timer->timerHandler->type() == Timer::Type::SingleShot && !timer->timerRef)
        {
            timer->create(*self);
        }
    };
    timers.forEach(loop);
}

TimerSourcePtr Adaptation::createTimerSource(std::string_view name)
{
    return TimerSourcePtr(new CFTimerSource(name));
}

}
