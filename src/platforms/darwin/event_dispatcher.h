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

template <class TDerived>
class RunLoopSource
{
    using Self = RunLoopSource<TDerived>;

public:
    enum RunLoopSourcePriority
    {
        kHighestPriority = 0
    };

    RunLoopSource()
    {
        CFRunLoopSourceContext context = {};
        context.info = this;
        context.perform = RunLoopSource::process;

        m_source = CFRunLoopSourceCreate(kCFAllocatorDefault, kHighestPriority, &context);
    }

    virtual ~RunLoopSource()
    {
        CFRunLoopSourceInvalidate(m_source);
        CFRelease(m_source);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
        {
            runLoop = CFRunLoopGetCurrent();
        }

        CFRunLoopAddSource(runLoop, m_source, mode);
    }

    void wakeUp()
    {
        CFRunLoopSourceSignal(m_source);
    }

protected:
    virtual void dispatch() {}

private:
    static void process(void *info)
    {
        Self* self = static_cast<Self*>(info);
        self->dispatch();
    }

    CFRunLoopSourceRef m_source;
};

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



class FoundationConcept : public Lockable
{
public:

    class PostEventSource : public RunLoopSource<PostEventSource>
    {
        RunLoopBase::EventProcessingCallback dispatcher;

    public:
        explicit PostEventSource() = default;
        void setDispatcher(RunLoopBase::EventProcessingCallback dispatcher)
        {
            this->dispatcher = dispatcher;
        }
        void dispatch() override
        {
            dispatcher();
        }
    };

    struct TimerRecord
    {
        TimerCorePtr timer;
        CFRunLoopTimerRef timerRef = nullptr;

        explicit TimerRecord(TimerCore& timer);
        ~TimerRecord();
        void start(FoundationConcept& source);
        void stop();
    };
    using TimerRecordPtr = std::unique_ptr<TimerRecord>;

    // TODO: use RunLoopSource<>
    struct SocketRecord
    {
        static void callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef, const void *, void *info);

        FoundationConcept& concept;
        CFSocketRef cfSocket = nullptr;
        CFRunLoopSourceRef cfSource = nullptr;
        SharedVector<SocketNotifierCorePtr> notifiers;
        SocketNotifier::EventTarget handler = -1;
        int readNotifierCount = 0;
        int writeNotifierCount = 0;

        SocketRecord(FoundationConcept& concept, SocketNotifierCore& notifier);
        ~SocketRecord();
        void addNotifier(SocketNotifierCore& notifier);
        // Returns true if the socket is releasable
        bool removeNotifier(SocketNotifierCore& notifier);
    };
    using SocketRecordPtr = std::unique_ptr<SocketRecord>;

    explicit FoundationConcept(RunLoopBase& runLoop);
    virtual ~FoundationConcept() = default;

protected:
    using IdleStack = std::stack<IdleFunction>;

    size_t runIdleTasks();
    void processRunLoopActivity(CFRunLoopActivity activity);

    void activateTimers();
    void stopTimers();

    void enableSocketNotifiers();
    void clearSocketNotifiers();

    struct NullTimer
    {
        bool operator()(TimerRecordPtr const& record) const
        {
            return !record || !record->timer;
        }
    };
    using TimerCollection = SharedVector<TimerRecordPtr, NullTimer>;

    RunLoopBase& self;
    mac::CFType<CFRunLoopRef> runLoop;
    RunLoopModeTracker *modeTracker = nullptr;
    CFStringRef currentMode = nullptr;

    RunLoopObserver<FoundationConcept> runLoopActivitySource;
    PostEventSource postEventSource;
    TimerCollection timers;
    std::vector<SocketRecordPtr> sockets;
    IdleStack idles;
};

template <class DerivedType, class LoopType>
class FoundationBase : public LoopType, public FoundationConcept
{
    using ThisType = FoundationBase<DerivedType, LoopType>;
    DerivedType* getThis()
    {
        return static_cast<DerivedType*>(this);
    }

    void dispatchEvenets()
    {
        getThis()->m_processEventsCallback();
    }

public:
    explicit FoundationBase()
        : FoundationConcept(static_cast<RunLoopBase&>(*this))
    {
        postEventSource.setDispatcher(std::bind(&ThisType::dispatchEvenets, this));
        currentMode = kCFRunLoopCommonModes;
    }

protected:
    void startTimerOverride(TimerCore& timer) final
    {
        lock_guard lock(*getThis());
        auto predicate = [&timer](TimerRecordPtr& trec)
        {
            return trec && trec->timer.get() == &timer;
        };
        timers.emplace_back_if(std::make_unique<TimerRecord>(timer), predicate);
    }

    void removeTimerOverride(TimerCore& timer) final
    {
        lock_guard lock(*getThis());
        auto eraser = [&timer](auto& trec)
        {
            if (!trec || trec->timer.get() != &timer)
            {
                return false;
            }

            trec->stop();
            return true;
        };
        erase_if(timers, eraser);
    }

    void attachSocketNotifierOverride(SocketNotifierCore& notifier) final
    {
        lock_guard lock(*getThis());
        auto lookup = [fd = notifier.handler()](auto& socket)
        {
            return socket->handler == fd;
        };
        auto it = find_if(sockets, lookup);
        if (it == sockets.end())
        {
            sockets.emplace_back(std::make_unique<SocketRecord>(*getThis(), notifier));
            CTRACE(event, "Socket::" << getThis()->sockets.back().get());
        }
        else
        {

            // Add this notifier
            (*it)->addNotifier(notifier);
        }
    }

    void detachSocketNotifierOverride(SocketNotifierCore& notifier) final
    {
        lock_guard lock(*getThis());
        auto lookup = [fd = notifier.handler()](auto& socket)
        {
            return socket->handler == fd;
        };
        auto socket = std::find_if(sockets.begin(), sockets.end(), lookup);
        if (socket == sockets.end())
        {
            return;
        }

        if ((*socket)->removeNotifier(notifier))
        {
            // remove the socket from the pool
            sockets.erase(socket);
        }
    }

    void scheduleSourcesOverride() final
    {
        lock_guard lock(*getThis());
        CTRACE(event, "WakeUp...");
        CFRunLoopWakeUp(runLoop);
        postEventSource.wakeUp();
    }

    void onIdleOverride(IdleFunction&& idle) final
    {
        lock_guard lock(*getThis());
        idles.emplace(std::forward<IdleFunction>(idle));
    }
};

class FoundationRunLoop : public FoundationBase<FoundationRunLoop, RunLoop>
{
public:
    explicit FoundationRunLoop() = default;

    void stop()
    {
        stopRunLoop();
    }

    void execute(ProcessFlags flags) final;
    void stopRunLoop() final;
};

class FoundationRunLoopHook : public FoundationBase<FoundationRunLoopHook, RunLoopHook>
{
public:
    explicit FoundationRunLoopHook();

    void stop()
    {
        stopRunLoop();
    }

    void stopRunLoop() final;
};

}

#endif // EVENT_LOOP_H
