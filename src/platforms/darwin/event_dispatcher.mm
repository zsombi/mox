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
            TRACE("Tried to pop run loop mode" << mode << "that was never pushed!");
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

FoundationRunLoop::FoundationRunLoop()
    : runLoopActivitySource(this, &FoundationRunLoop::processRunLoopActivity, kCFRunLoopAllActivities)
    , runLoop(mac::CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent()))
    , modeTracker([[RunLoopModeTracker alloc] init])
{
    runLoopActivitySource.addToMode(kCFRunLoopCommonModes);
}

FoundationRunLoop::~FoundationRunLoop()
{
}

void FoundationRunLoop::scheduleIdleTasks()
{
    wakeUp();
}

void FoundationRunLoop::processRunLoopActivity(CFRunLoopActivity activity)
{
    switch (activity)
    {
        case kCFRunLoopEntry:
        {
            TRACE("Entering runloop");
            break;
        }
        case kCFRunLoopBeforeTimers:
        {
            if (!m_runOnce)
            {
                TRACE("Before timers...");
                forEachSource<CFTimerSource>(&CFTimerSource::activate);
            }
            break;
        }
        case kCFRunLoopBeforeSources:
        {
            TRACE("Before sources...");
            forEachSource<CFSocketNotifierSource>(&CFSocketNotifierSource::enableSockets);
            break;
        }
        case kCFRunLoopBeforeWaiting:
        {
            TRACE("RunLoop is about to sleep, run idle tasks");
            // Run idle tasks
            if (runIdleTasks())
            {
                scheduleIdleTasks();
            }
            if (m_runOnce && !runningTimerCount())
            {
                TRACE("runOnce invoked");
                stopExecution();
            }
            break;
        }
        case kCFRunLoopAfterWaiting:
        {
            TRACE("After waiting, resumed");
            break;
        }
        case kCFRunLoopExit:
        {
            if (!m_runOnce)
            {
                TRACE("Exiting");
                forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::clean);
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
RunLoopSharedPtr Adaptation::createRunLoop(bool main)
{
    UNUSED(main);
    TRACE("Run loop for main?" << main);
    return make_polymorphic_shared<RunLoop, FoundationRunLoop>();
}

bool FoundationRunLoop::isRunning() const
{
    return m_isRunning;
}

void FoundationRunLoop::execute(ProcessFlags)
{
    FlagScope<true> toggleRunning(m_isRunning);
    currentMode = kCFRunLoopCommonModes;
    FlagScope<false> lockRunOnce(m_runOnce);
    CFRunLoopRun();
}

void FoundationRunLoop::runOnce()
{
    constexpr CFTimeInterval kCFTimeIntervalMinimum = 0;
    constexpr CFTimeInterval kCFTimeIntervalDistantFuture = std::numeric_limits<CFTimeInterval>::max();
    constexpr bool returnAfterSingleSourceHandled = false;
    constexpr bool waitForEvents = true;

    currentMode = [modeTracker currentMode];
    CFTimeInterval duration = waitForEvents ? kCFTimeIntervalDistantFuture : kCFTimeIntervalMinimum;
    SInt32 result = CFRunLoopRunInMode(currentMode, duration, returnAfterSingleSourceHandled);
    TRACE("result in mode [" << currentMode << "]:" << result << ", duration:" << duration);
    UNUSED(result);
}

void FoundationRunLoop::stopExecution()
{
    TRACE("Stop run loop");
    CFRunLoopStop(runLoop);
}

void FoundationRunLoop::wakeUp()
{
    forEachSource<EventSource>(&EventSource::wakeUp);
    if (runLoop)
    {
        TRACE("WakeUp...");
        CFRunLoopWakeUp(runLoop);
    }
}

}
