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

CFTimerSource::CFTimerRecord::CFTimerRecord(TimerRecord& handler)
    : timerHandler(handler.shared_from_this())
{
}

CFTimerSource::CFTimerRecord::~CFTimerRecord()
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

void CFTimerSource::CFTimerRecord::create(CFTimerSource& source)
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
    CFAbsoluteTime timeout = CFAbsoluteTime(timerHandler->getInterval().count()) / 1000;
    CFAbsoluteTime timeToFire = CFAbsoluteTimeGetCurrent() + timeout;
    CFAbsoluteTime interval = timerHandler->isSingleShot() ? -1 : timeout;

    auto proc = [&source, self = this](CFRunLoopTimerRef)
    {
        if (!self->timerHandler)
        {
            return;
        }

        lock_guard lock(source.timers);
        TRACE("Signaling timer " << self->timerRef << " of self[" << self << ']' << source.timers.lockCount())
        TimerPtr keepAlive = self->timerHandler;
        keepAlive->signal();
        TRACE("Timer signaled: " << self->timerRef << " of self[" << self << ']' << source.timers.lockCount())
    };
    timerRef = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, timeToFire, interval, 0, 0, proc);
    FoundationRunLoop* foundationLoop = static_cast<FoundationRunLoop*>(source.getRunLoop().get());
    CFRunLoopAddTimer(foundationLoop->runLoop, timerRef, kCFRunLoopCommonModes);
    FATAL(CFRunLoopTimerIsValid(timerRef), "Invalid timer created!")
    TRACE("Timer created with interval: " << interval << ", timeout " << timeout << ", ref: " << timerRef)
}

/******************************************************************************
 * FoundationRunLoop
 */
size_t FoundationRunLoop::runningTimerCount() const
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

void CFTimerSource::addTimer(TimerRecord& timer)
{
    auto predicate = [&timer](CFTimerRecordPtr& record)
    {
        return record && record->timerHandler.get() == &timer;
    };
    auto index = timers.findIf(predicate);
    FATAL(!index, "Timer already registered")

    lock_guard lock(timers);
    timers.emplace_back(std::make_unique<CFTimerRecord>(timer));
}

void CFTimerSource::removeTimer(TimerRecord& timer)
{
    auto predicate = [&timer](CFTimerRecordPtr& record)
    {
        return record && record->timerHandler.get() == &timer;
    };
    auto index = timers.findIf(predicate);
    FATAL(index, "Timer not registered")

    lock_guard lock(timers);
    timers[*index]->timerHandler.reset();
    timers[*index].reset();
}

void CFTimerSource::clean()
{
    lock_guard lock(timers);
    auto looper = [](CFTimerRecordPtr& timer)
    {
        if (timer && timer->timerHandler)
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
    auto loop = [self = this](CFTimerRecordPtr& timer)
    {
        if (!timer || !timer->timerHandler)
        {
            return;
        }

        if (!timer->timerHandler->isSingleShot())
        {
            // Recreate timeRef if invalid.
            if (!timer->timerRef || !CFRunLoopTimerIsValid(timer->timerRef))
            {
                TRACE("Recreate repeating timer")
                timer->create(*self);
            }
        }

        if (timer->timerHandler->isSingleShot() && !timer->timerRef)
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
