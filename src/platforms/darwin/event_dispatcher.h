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

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/socket_notifier.hpp>
#include <mox/timer.hpp>
#include <mox/utils/containers.hpp>
#include <mox/platforms/adaptation.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include "mac_util.h"

namespace mox
{

class CFEventDispatcher;

template <class T = CFEventDispatcher>
class RunLoopSource
{
public:
    typedef void (T::*Callback)();

    enum { HighestPriority = 0 };

    explicit RunLoopSource(T* delegate, Callback callback)
        : m_delegate(delegate)
        , m_callback(callback)
    {
        CFRunLoopSourceContext context = {};
        context.info = this;
        context.perform = RunLoopSource::process;

        m_sourceRef = CFRunLoopSourceCreate(kCFAllocatorDefault, HighestPriority, &context);
        FATAL(m_sourceRef, "CF runloop source creation failed")
    }
    ~RunLoopSource()
    {
        CFRunLoopSourceInvalidate(m_sourceRef);
        CFRelease(m_sourceRef);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef loop = nullptr)
    {
        if (!loop)
        {
            loop = CFRunLoopGetCurrent();
        }
        CFRunLoopAddSource(loop, m_sourceRef, mode);
    }

    void removeFromMode(CFStringRef mode, CFRunLoopRef loop = nullptr)
    {
        if (!loop)
        {
            loop = CFRunLoopGetCurrent();
        }
        if (!CFRunLoopContainsSource(loop, m_sourceRef, mode))
        {
            CFRunLoopRemoveSource(loop, m_sourceRef, mode);
        }
    }

    void signal()
    {
        CFRunLoopSourceSignal(m_sourceRef);
    }

private:
    static void process(void *info)
    {
        RunLoopSource* self = static_cast<RunLoopSource*>(info);
        ((self->m_delegate)->*(self->m_callback))();
    }

    T* m_delegate = nullptr;
    Callback m_callback = nullptr;
    CFRunLoopSourceRef m_sourceRef;
};

template <class T = CFEventDispatcher>
class RunLoopObserver
{
public:
    typedef void (T::*Callback)(CFRunLoopActivity);

    explicit RunLoopObserver(T* delegate, Callback callback, CFOptionFlags activities)
        : m_delegate(delegate)
        , m_callback(callback)
    {
        CFRunLoopObserverContext context = {};
        context.info = this;

        m_observerRef = CFRunLoopObserverCreate(kCFAllocatorDefault, activities, true, 0, process, &context);
        FATAL(m_observerRef, "CF observer creation failed")
    }
    ~RunLoopObserver()
    {
        CFRunLoopObserverInvalidate(m_observerRef);
        CFRelease(m_observerRef);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef loop = nullptr)
    {
        if (!loop)
        {
            loop = CFRunLoopGetCurrent();
        }
        if (!CFRunLoopContainsObserver(loop, m_observerRef, mode))
        {
            CFRunLoopAddObserver(loop, m_observerRef, mode);
        }
    }

    void removeFromMode(CFStringRef mode, CFRunLoopRef loop = nullptr)
    {
        if (!loop)
        {
            loop = CFRunLoopGetCurrent();
        }
        if (!CFRunLoopContainsObserver(loop, m_observerRef, mode))
        {
            CFRunLoopRemoveObserver(loop, m_observerRef, mode);
        }
    }

private:
    static void process(CFRunLoopObserverRef, CFRunLoopActivity activity, void *info)
    {
        RunLoopObserver* self = static_cast<RunLoopObserver*>(info);
        ((self->m_delegate)->*(self->m_callback))(activity);
    }

    T* m_delegate = nullptr;
    Callback m_callback = nullptr;
    CFRunLoopObserverRef m_observerRef;
};

class CFTimerSource : public TimerSource
{
public:
    struct TimerRecord
    {
        CFRunLoopTimerRef timerRef = nullptr;
        TimerPtr timerHandler;

        TimerRecord(Timer& handler);
        ~TimerRecord();

        void create(CFTimerSource& source);
    };

    using TimerRecordPtr = std::unique_ptr<TimerRecord>;

    explicit CFTimerSource(std::string_view name)
        : TimerSource(name)
        , timers([] (TimerRecordPtr const& timer) { return timer->timerHandler == nullptr; })
    {
    }

    void addTimer(Timer& timer) final;
    void removeTimer(Timer &timer) final;
    void shutDown() final;
    size_t timerCount() const final;

    void activate();

    LockableContainer<TimerRecordPtr> timers;
};

class CFPostEventSource : public PostEventSource
{
public:
    explicit CFPostEventSource(std::string_view name);
    ~CFPostEventSource() final;
    void setEventDispatcher(EventDispatcher& eventDispatcher) final;

    void wakeUp() override;

    CFRunLoopSourceRef sourceRef = nullptr;
};

class CFSocketNotifierSource : public SocketNotifierSource
{
public:
    explicit CFSocketNotifierSource(std::string_view name);
    ~CFSocketNotifierSource() final;

    void prepare() final;
    void shutDown() final;
    void addNotifier(SocketNotifier &notifier) final;
    void removeNotifier(SocketNotifier &notifier) final;

    void enableSockets();

    // internals
    struct Socket
    {
        CFSocketNotifierSource& socketSource;
        CFSocketRef cfSocket = nullptr;
        CFRunLoopSourceRef cfSource = nullptr;
        LockableContainer<SocketNotifierSharedPtr> notifiers;
        SocketNotifier::Handler handler = -1;
        int readNotifierCount = 0;
        int writeNotifierCount = 0;

        Socket(CFSocketNotifierSource& socketSource, SocketNotifier& notifier);
        ~Socket();
        void addNotifier(SocketNotifier& notifier);
        // Returns true if the socket is releasable
        bool removeNotifier(SocketNotifier& notifier);

        static void callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef, const void *, void *info);
    };
    using SocketPtr = std::unique_ptr<Socket>;

    std::vector<std::unique_ptr<Socket>> sockets;
};

class CFEventDispatcher : public EventDispatcher
{
public:
    explicit CFEventDispatcher();
    ~CFEventDispatcher() override;

    int processEvents(ProcessFlags flags) override;
    void exit(int exitCode = 0) override;
    void wakeUp() override;
    size_t runningTimerCount() const override;

    void scheduleIdleTasks() override;

    void runOnce();
    void processRunLoopActivity(CFRunLoopActivity activity);

    RunLoopObserver<> runLoopActivitySource;
    CFType<CFRunLoopRef> runLoop;
    RunLoopModeTracker *modeTracker = nullptr;
    CFStringRef currentMode = nullptr;
    bool m_runOnce = true;
};

}

#endif // EVENT_LOOP_H
