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
#include <mox/utils/log/logger.hpp>

namespace mox
{

/******************************************************************************
 * TimerRecord
 */

FoundationConcept::TimerRecord::TimerRecord(TimerCore& timer)
    : timer(timer.shared_from_this())
{
}

FoundationConcept::TimerRecord::~TimerRecord()
{
    stop();
}

void FoundationConcept::TimerRecord::start(FoundationConcept& source)
{
    CTRACE(event, "Create timer record for source");
    if (!timer || (timerRef && CFRunLoopTimerIsValid(timerRef)))
    {
        return;
    }
    if (timerRef)
    {
        CWARN(event, "recreating timer?!");
        CFRelease(timerRef);
    }
    CFAbsoluteTime timeout = CFAbsoluteTime(timer->getInterval().count()) / 1000;
    CFAbsoluteTime timeToFire = CFAbsoluteTimeGetCurrent() + timeout;
    CFAbsoluteTime interval = timer->isSingleShot() ? -1 : timeout;

    auto proc = [&source, self = this](CFRunLoopTimerRef)
    {
        if (!self->timer)
        {
            return;
        }

        lock_guard lock(source.timers);
        CTRACE(event, "Signaling timer" << self->timerRef << "of self[" << self << ']' << source.timers.lockCount());
        auto keepAlive = self->timer;
        keepAlive->signal();
        CTRACE(event, "Timer signaled:" << self->timerRef << "of self[" << self << ']' << source.timers.lockCount());
    };
    timerRef = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, timeToFire, interval, 0, 0, proc);
    CFRunLoopAddTimer(source.runLoop, timerRef, kCFRunLoopCommonModes);
    CTRACE(event, "Timer created with interval:" << interval << ", timeout" << timeout << ", ref:" << timerRef);
}

void FoundationConcept::TimerRecord::stop()
{
    if (timerRef)
    {
        CTRACE(event, "Deleting timer:" << timerRef);
        CFRunLoopTimerInvalidate(timerRef);
        CFRelease(timerRef);
        timerRef = nullptr;
    }
    timer.reset();
}

/******************************************************************************
 *
 */
void FoundationConcept::activateTimers()
{
    lock_guard lock(*this);
    auto loop = [self = this](TimerRecordPtr& trec)
    {
        if (!trec || !trec->timer)
        {
            return;
        }

        if (!trec->timer->isSingleShot())
        {
            // Recreate timeRef if invalid.
            if (!trec->timerRef || !CFRunLoopTimerIsValid(trec->timerRef))
            {
                CTRACE(event, "Recreate repeating timer");
                trec->start(*self);
            }
        }

        if (trec->timer->isSingleShot() && !trec->timerRef)
        {
            trec->start(*self);
        }
    };
    for_each(timers, loop);
}

void FoundationConcept::stopTimers()
{
    lock_guard lock(*this);
    auto looper = [self = this](TimerRecordPtr& trec)
    {
        if (trec && trec->timer)
        {
            ScopeRelock re(*self);
            trec->timer->stop();
        }
    };
    for_each(timers, looper);
}

}
