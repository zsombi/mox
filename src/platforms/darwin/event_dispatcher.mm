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
            CTRACE(event, "Encountered run loop push notification without run loop mode!");
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
            CTRACE(event, "Tried to pop run loop mode" << mode << "that was never pushed!");
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

/******************************************************************************
 * FoundationConcept
 */
FoundationConcept::FoundationConcept(RunLoopBase& runLoopBase)
    : self(runLoopBase)
    , runLoop(mac::CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent()))
    , modeTracker([[RunLoopModeTracker alloc] init])
    , runLoopActivitySource(this, &FoundationConcept::processRunLoopActivity, kCFRunLoopAllActivities)
{
    runLoopActivitySource.addToMode(kCFRunLoopCommonModes, runLoop);
    postEventSource.addToMode(kCFRunLoopCommonModes, runLoop);
}

void FoundationConcept::processRunLoopActivity(CFRunLoopActivity activity)
{
    switch (activity)
    {
        case kCFRunLoopEntry:
        {
            CTRACE(event, "Entering runloop");
            break;
        }
        case kCFRunLoopBeforeTimers:
        {
            CTRACE(event, "Before timers...");
            activateTimers();
            break;
        }
        case kCFRunLoopBeforeSources:
        {
            CTRACE(event, "Before sources...");
            enableSocketNotifiers();
            break;
        }
        case kCFRunLoopBeforeWaiting:
        {
            CTRACE(event, "RunLoop is about to sleep, run idle tasks");
            // Run idle tasks
            if (runIdleTasks() > 0u && (self.getStatus() < RunLoopBase::Status::Exiting))
            {
                self.scheduleSources();
            }
            break;
        }
        case kCFRunLoopAfterWaiting:
        {
            CTRACE(event, "After waiting, resumed");
            break;
        }
        case kCFRunLoopExit:
        {
            CTRACE(event, "Exiting");
            clearSocketNotifiers();
            stopTimers();

            self.quit();
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
 * FoundationRunLoop
 */
void FoundationRunLoop::execute(ProcessFlags flags)
{
    if (m_status > Status::Running)
    {
        return;
    }
    if (flags != ProcessFlags::SingleLoop)
    {
        m_status = Status::Running;
        CFRunLoopRun();
    }

    notifyRunLoopDown();
}

void FoundationRunLoop::stopRunLoop()
{
    lock_guard lock(*this);
    CTRACE(event, "Stop run loop");
    CFRunLoopStop(runLoop);
    m_status = Status::Exiting;
}

/******************************************************************************
 * FoundationRunLoopHook
 */
FoundationRunLoopHook::FoundationRunLoopHook()
{
    m_status = Status::Running;
}

void FoundationRunLoopHook::stopRunLoop()
{
    m_status = Status::Exiting;
    notifyRunLoopDown();
}

/******************************************************************************
 * Adaptation
 */
RunLoopPtr Adaptation::createRunLoop(bool main)
{
    UNUSED(main);
    CTRACE(event, "Run loop for main?" << main);
    return make_polymorphic_shared<RunLoop, FoundationRunLoop>();
}

RunLoopHookPtr Adaptation::createRunLoopHook()
{
    return make_polymorphic_shared<RunLoopHook, FoundationRunLoopHook>();
}

}
