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
FoundationConcept::FoundationConcept()
    : runLoop(mac::CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent()))
    , modeTracker([[RunLoopModeTracker alloc] init])
{
}

void FoundationConcept::addSource(CFRunLoopSourceRef source)
{
    CFRunLoopAddSource(runLoop, source, currentMode);
}

void FoundationConcept::addTimerSource(CFRunLoopTimerRef timer)
{
    CFRunLoopAddTimer(runLoop, timer, kCFRunLoopCommonModes);
}

void FoundationConcept::removeSource(CFRunLoopSourceRef source, CFRunLoopMode mode)
{
    CFRunLoopRemoveSource(runLoop, source, mode);
}

/******************************************************************************
 * FoundationRunLoop
 */
void FoundationRunLoop::execute(ProcessFlags flags)
{
    if (isExiting() || m_isRunning)
    {
        return;
    }
    if (flags != ProcessFlags::SingleLoop)
    {
        ScopeValue toggleRunning(m_isRunning, true);
        currentMode = kCFRunLoopCommonModes;
        CFRunLoopRun();
    }

    notifyRunLoopDown();
}

void FoundationRunLoop::stopRunLoop()
{
    CTRACE(event, "Stop run loop");
    CFRunLoopStop(runLoop);
}

/******************************************************************************
 * FoundationRunLoopHook
 */
void FoundationRunLoopHook::onEnter()
{
    currentMode = kCFRunLoopCommonModes;
    m_isRunning = true;
}

void FoundationRunLoopHook::onExit()
{
    if (m_isRunning)
    {
        quit();
    }
    m_isRunning = false;
}

void FoundationRunLoopHook::stopRunLoop()
{
    notifyRunLoopDown();
}

/******************************************************************************
 * Adaptation
 */
RunLoopSharedPtr Adaptation::createRunLoop(bool main)
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
