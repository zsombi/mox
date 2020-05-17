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

#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/meta/class/metaobject.hpp>
#include <mox/core/event_handling/socket_notifier.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <mox/core/platforms/adaptation.hpp>

#include <stack>
#include <CoreFoundation/CoreFoundation.h>
#include "mac_util.h"

@interface RunLoopModeTracker :NSObject
@end

namespace mox
{

template <class T>
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
        FATAL(m_observerRef, "CF observer creation failed");
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
    struct CFTimerRecord
    {
        CFRunLoopTimerRef timerRef = nullptr;
        TimerSource::TimerPtr timerHandler;

        CFTimerRecord(TimerRecord& handler);
        ~CFTimerRecord();

        void create(CFTimerSource& source);
    };

    using CFTimerRecordPtr = std::unique_ptr<CFTimerRecord>;

    explicit CFTimerSource(std::string_view name)
        : TimerSource(name)
    {
    }

    void addTimer(TimerRecord& timer) final;
    void removeTimer(TimerRecord& timer) final;
    void initialize(void*) final {}
    void detachOverride() final;
    size_t timerCount() const final;
    void activate();

    struct NullTimer
    {
        bool operator()(CFTimerRecordPtr const& timer) const
        {
            return !timer || timer->timerHandler == nullptr;
        }
    };

    using TimerCollection = SharedVector<CFTimerRecordPtr, NullTimer>;

    TimerCollection timers;
};

class CFPostEventSource : public EventSource
{
public:
    explicit CFPostEventSource(std::string_view name);
    ~CFPostEventSource() final;
    void initialize(void* data) final;
    void detachOverride() final;

    void wakeUp() override;

    CFRunLoopSourceRef sourceRef = nullptr;
};

class CFSocketNotifierSource : public SocketNotifierSource
{
public:
    explicit CFSocketNotifierSource(std::string_view name);
    ~CFSocketNotifierSource() final;

    void initialize(void*) final {}
    void detachOverride() final;
    void addNotifier(Notifier &notifier) final;
    void removeNotifier(Notifier &notifier) final;

    void enableSockets();

    // internals
    struct Socket
    {
        CFSocketNotifierSource& socketSource;
        CFSocketRef cfSocket = nullptr;
        CFRunLoopSourceRef cfSource = nullptr;
        SharedVector<NotifierPtr> notifiers;
        SocketNotifier::EventTarget handler = -1;
        int readNotifierCount = 0;
        int writeNotifierCount = 0;

        Socket(CFSocketNotifierSource& socketSource, Notifier& notifier);
        ~Socket();
        void addNotifier(Notifier& notifier);
        // Returns true if the socket is releasable
        bool removeNotifier(Notifier& notifier);

        static void callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef, const void *, void *info);
    };
    using SocketPtr = std::unique_ptr<Socket>;

    std::vector<std::unique_ptr<Socket>> sockets;
};

class CFIdleSource : public IdleSource
{
    using TaskStack = std::stack<Task>;
    TaskStack tasks;

public:
    explicit CFIdleSource() = default;
    void initialize(void*) final;
    void detachOverride() final;
    void wakeUp() final;

    /// Run the idle tasks. Returns the number of re-scheduled idle tasks in the stack.
    size_t runTasks();

protected:
    void addIdleTaskOverride(Task&& task) override;
};

class FoundationConcept
{
public:
    explicit FoundationConcept();
    virtual ~FoundationConcept() = default;

    void addSource(CFRunLoopSourceRef source);
    void addTimerSource(CFRunLoopTimerRef timer);
    void removeSource(CFRunLoopSourceRef source, CFRunLoopMode mode = kCFRunLoopCommonModes);

protected:
    mac::CFType<CFRunLoopRef> runLoop;
    RunLoopModeTracker *modeTracker = nullptr;
    CFStringRef currentMode = nullptr;
};

template <class DerivedType, class LoopType>
class FoundationBase : public LoopType, public FoundationConcept
{
    using ThisType = FoundationBase<DerivedType, LoopType>;
    DerivedType* getThis()
    {
        return static_cast<DerivedType*>(this);
    }

    IdleSourceWeakPtr idleSource;

public:
    explicit FoundationBase()
        : runLoopActivitySource(this, &FoundationBase::processRunLoopActivity, kCFRunLoopAllActivities)
    {
        runLoopActivitySource.addToMode(kCFRunLoopCommonModes);
    }

    bool isRunningOverride() const final
    {
        return m_isRunning;
    }

    void initialize() final
    {
        auto self = getThis();
        void* data = (void*)runLoop;
        self->template forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::initialize, data);
        idleSource = self->getIdleSource();
    }

protected:
    void scheduleSourcesOverride() final
    {
        CTRACE(event, "WakeUp...");
        CFRunLoopWakeUp(runLoop);
    }

    void processRunLoopActivity(CFRunLoopActivity activity)
    {
        auto self = getThis();

        switch (activity)
        {
            case kCFRunLoopEntry:
            {
                CTRACE(event, "Entering runloop");
                self->onEnter();
                break;
            }
            case kCFRunLoopBeforeTimers:
            {
                CTRACE(event, "Before timers...");
                self->template forEachSource<CFTimerSource>(&CFTimerSource::activate);
                break;
            }
            case kCFRunLoopBeforeSources:
            {
                CTRACE(event, "Before sources...");
                self->template forEachSource<CFSocketNotifierSource>(&CFSocketNotifierSource::enableSockets);
                break;
            }
            case kCFRunLoopBeforeWaiting:
            {
                CTRACE(event, "RunLoop is about to sleep, run idle tasks");
                // Run idle tasks
                if (self->isRunning() && !idleSource.expired())
                {
                    auto idle = std::dynamic_pointer_cast<CFIdleSource>(idleSource.lock());
                    if (idle->runTasks() > 0u && !self->isExiting())
                    {
                        self->scheduleSources();
                    }
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
                getThis()->onExit();
                break;
            }
            default:
            {
                break;
            }
        }
    }

    RunLoopObserver<ThisType> runLoopActivitySource;
    std::atomic_bool m_isRunning = false;
};

class FoundationRunLoop : public FoundationBase<FoundationRunLoop, RunLoop>
{
public:
    explicit FoundationRunLoop() = default;

    void stop()
    {
        stopRunLoop();
    }

    void onEnter() {}
    void onExit() {}

    void execute(ProcessFlags flags) final;
    void stopRunLoop() final;
};

class FoundationRunLoopHook : public FoundationBase<FoundationRunLoopHook, RunLoopHook>
{
public:
    explicit FoundationRunLoopHook() = default;

    void stop()
    {
        stopRunLoop();
    }

    void onEnter();
    void onExit();

    void stopRunLoop() final;
};

}

#endif // EVENT_LOOP_H
