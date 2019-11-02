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
#include <mox/event_handling/event_loop.hpp>
#include <stack>

@interface RunLoopModeTracker :NSObject
@end

@implementation RunLoopModeTracker
{
    std::stack<CFStringRef> m_runLoopModes;
}

- (instancetype)init
{
    if ((self = [super init]))
    {
        m_runLoopModes.push(kCFRunLoopDefaultMode);
    }

    return self;
}

- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];

    [super dealloc];
}

static CFStringRef runLoopMode(NSDictionary *dictionary)
{
    for (NSString *key in dictionary)
    {
        if (CFStringHasSuffix((CFStringRef)key, CFSTR("RunLoopMode")))
        {
            return (CFStringRef)dictionary[key];
        }
    }

    return nil;
}

- (void)receivedNotification:(NSNotification *)notification
{
    if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePushNotification")))
    {
        if (CFStringRef mode = runLoopMode(notification.userInfo))
        {
            m_runLoopModes.push(mode);
        }
        else
        {
            TRACE("Encountered run loop push notification without run loop mode!");
        }

    }
    else if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePopNotification")))
    {
        CFStringRef mode = runLoopMode(notification.userInfo);
        if (CFStringCompare(mode, self.currentMode, 0) == kCFCompareEqualTo)
        {
            m_runLoopModes.pop();
        }
        else
        {
            TRACE("Tried to pop run loop mode" << /*qPrintable(QString::fromCFString(*/mode/*))*/ << "that was never pushed!");
        }

        FATAL(m_runLoopModes.size() >= 1, "Broken runloop modes");
    }
}

- (CFStringRef)currentMode
{
    return m_runLoopModes.top();
}

@end

namespace mox
{

CFEventDispatcher::CFEventDispatcher(ThreadData& threadData)
    : EventDispatcher(threadData)
    , runLoopActivitySource(this, &CFEventDispatcher::processRunLoopActivity, kCFRunLoopAllActivities)
    , runLoop(CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent()))
    , modeTracker([[RunLoopModeTracker alloc] init])
{
    runLoopActivitySource.addToMode(kCFRunLoopCommonModes);
}

CFEventDispatcher::~CFEventDispatcher()
{
}

void CFEventDispatcher::scheduleIdleTasks()
{
    wakeUp();
}

void CFEventDispatcher::processRunLoopActivity(CFRunLoopActivity activity)
{
    switch (activity)
    {
        case kCFRunLoopEntry:
        {
            TRACE("Entering runloop")
            break;
        }
        case kCFRunLoopBeforeTimers:
        {
            if (!m_runOnce)
            {
                TRACE("Before timers...")
                forEachSource<CFTimerSource>(&CFTimerSource::activate);
            }
            break;
        }
        case kCFRunLoopBeforeSources:
        {
            TRACE("Before sources...")
            forEachSource<CFSocketNotifierSource>(&CFSocketNotifierSource::enableSockets);
            break;
        }
        case kCFRunLoopBeforeWaiting:
        {
            TRACE("RunLoop is about to sleep, run idle tasks")
            // Run idle tasks
            if (runIdleTasks())
            {
                scheduleIdleTasks();
            }
            if (m_runOnce && !runningTimerCount())
            {
                TRACE("runOnce invoked")
                stop();
            }
            break;
        }
        case kCFRunLoopAfterWaiting:
        {
            TRACE("After waiting, resumed")
            break;
        }
        case kCFRunLoopExit:
        {
            if (!m_runOnce)
            {
                TRACE("Exiting")
                forEachSource<AbstractEventSource>(&AbstractEventSource::shutDown);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
 *
 */
EventDispatcherSharedPtr Adaptation::createEventDispatcher(ThreadData& threadData, bool)
{
    return make_polymorphic_shared<EventDispatcher, CFEventDispatcher>(threadData);
}

bool CFEventDispatcher::isProcessingEvents() const
{
    return m_isRunning;
}

void CFEventDispatcher::processEvents(ProcessFlags flags)
{
    FlagScope<true> toggleRunning(m_isRunning);
    if (flags == ProcessFlags::RunOnce)
    {
        TRACE("Entering runOnce()")
        runOnce();
        TRACE("Leaving runOnce()")
    }
    else
    {
        currentMode = kCFRunLoopCommonModes;
        FlagScope<false> lockRunOnce(m_runOnce);
        CFRunLoopRun();
    }
}

void CFEventDispatcher::runOnce()
{
    constexpr CFTimeInterval kCFTimeIntervalMinimum = 0;
    constexpr CFTimeInterval kCFTimeIntervalDistantFuture = std::numeric_limits<CFTimeInterval>::max();
    constexpr bool returnAfterSingleSourceHandled = false;
    constexpr bool waitForEvents = true;

    currentMode = [modeTracker currentMode];
    CFTimeInterval duration = waitForEvents ? kCFTimeIntervalDistantFuture : kCFTimeIntervalMinimum;
    SInt32 result = CFRunLoopRunInMode(currentMode, duration, returnAfterSingleSourceHandled);
    TRACE("result in mode [" << currentMode << "]: " << result << ", duration: " << duration)
    UNUSED(result);
}

void CFEventDispatcher::stop()
{
    TRACE("Stop dispatcher")
    CFRunLoopStop(runLoop);
}

void CFEventDispatcher::wakeUp()
{
    forEachSource<PostEventSource>(&PostEventSource::wakeUp);
    if (runLoop)
    {
        TRACE("WakeUp...")
        CFRunLoopWakeUp(runLoop);
    }
}

}
