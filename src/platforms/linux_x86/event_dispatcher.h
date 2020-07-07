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
#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/timer.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <mox/core/platforms/adaptation.hpp>

#include <glib.h>

namespace mox
{

class GlibRunLoop;

struct GPollHandler
{
    GPollFD fd;
    SocketNotifierCorePtr notifier;
    explicit GPollHandler(SocketNotifierCore& notifier);
    void reset();
};

class GlibRunLoopBase : public Lockable
{
public:
    explicit GlibRunLoopBase(GMainContext* mainContext);
    virtual ~GlibRunLoopBase();

    struct PostEventSource : GSource
    {
        GlibRunLoopBase* m_runLoop = nullptr;
        std::mutex m_lock;
        int m_serialNumber = 0;
        int m_lastSerialNumber = 0;

        void wakeUp();

        static gboolean prepare(GSource *src, gint* timeout);
        static gboolean dispatch(GSource* src, GSourceFunc, gpointer);

        static PostEventSource* create(GlibRunLoopBase* context);
        static void destroy(PostEventSource*& source);
    };

    struct TimerSource : GSource
    {
        TimerCorePtr m_timer;
        Timestamp m_lastUpdateTime;
        bool m_active = true;

        static gboolean prepare(GSource* src, gint* timeout);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static TimerSource* create(TimerCore& timer, GMainContext* context);
        static void destroy(TimerSource*& src);
    };

    struct SocketNotifierSource : GSource
    {
        static gboolean prepare(GSource* src, gint *timeout);
        static gboolean check(GSource* src);
        static gboolean dispatch(GSource* src, GSourceFunc, gpointer);

        static SocketNotifierSource* create(GlibRunLoopBase& self, GMainContext* context);
        static void destroy(SocketNotifierSource*& source);

        struct NullPollHandler
        {
            bool operator()(const GPollHandler& handler) const
            {
                return handler.notifier == nullptr;
            }
        };

        struct PollInvalidator
        {
            void operator()(GPollHandler& handler)
            {
                handler.notifier.reset();
            }
        };

        GlibRunLoopBase* m_self = nullptr;
        SharedVector<GPollHandler, NullPollHandler, PollInvalidator> m_pollHandlers;
    };

    struct IdleBundle
    {
        GMainContext* context = nullptr;
        IdleFunction idle;
        guint sourceId;

        explicit IdleBundle(GMainContext* context, IdleFunction&& idle);
        static gboolean callback(gpointer userData);
    };

protected:
    virtual void dispatchEvents() = 0;

    struct ZeroTimer
    {
        bool operator()(TimerSource* const& source) const
        {
            return !source || !source->m_timer;
        }
    };

    SharedVector<TimerSource*, ZeroTimer> timerSources;
    PostEventSource* postEventSource = nullptr;
    SocketNotifierSource* socketNotifierSource = nullptr;
    GMainContext* context = nullptr;
};

template <typename TDerived, typename TBase>
class GlibRunLoopImpl : public GlibRunLoopBase, public TBase
{
    TDerived* getThis()
    {
        return static_cast<TDerived*>(this);
    }
    TDerived* getThis() const
    {
        return static_cast<const TDerived*>(this);
    }

public:
    explicit GlibRunLoopImpl(GMainContext* mainContext)
        : GlibRunLoopBase(mainContext)
    {        
    }

    void startTimerOverride(TimerCore& timer) override
    {
        std::unique_lock locker(*this);
        // Make sure the timer is registered once.
        auto finder = [&timer](const auto* source)
        {
            return source->m_timer.get() == &timer;
        };
        auto gtimer = TimerSource::create(timer, getThis()->context);
        if (!getThis()->timerSources.push_back_if(gtimer, finder))
        {
            TimerSource::destroy(gtimer);
            CWARN(platform, "The timer is already registered");
            return;
        }    
    }

    void removeTimerOverride(TimerCore& timer) override
    {
        std::unique_lock locker(*this);
        auto eraser = [&timer](auto* source)
        {
            if (source->m_timer.get() == &timer)
            {
                TimerSource::destroy(source);
                return true;
            }
            return false;
        };
        erase_if(getThis()->timerSources, eraser);
    }

    void attachSocketNotifierOverride(SocketNotifierCore& notifier) override
    {
        std::unique_lock locker(*this);
        socketNotifierSource->m_pollHandlers.emplace_back(GPollHandler(notifier));
        g_source_add_poll(static_cast<GSource*>(socketNotifierSource), &socketNotifierSource->m_pollHandlers.back().fd);
    }

    void detachSocketNotifierOverride(SocketNotifierCore& notifier) override
    {
        std::unique_lock locker(*this);
        auto predicate = [&notifier, this](GPollHandler& poll)
        {
            if (poll.notifier.get() == &notifier)
            {
                g_source_remove_poll(static_cast<GSource*>(this->socketNotifierSource), &poll.fd);
                return true;
            }
            return false;
        };
        erase_if(socketNotifierSource->m_pollHandlers, predicate);
    }

    void onIdleOverride(IdleFunction&& idle) override
    {
        std::unique_lock locker(*this);
        new IdleBundle(context, std::forward<IdleFunction>(idle));
    }

protected:
    void dispatchEvents() override
    {
        getThis()->m_processEventsCallback();
    }

    void scheduleSourcesOverride() final
    {
        std::unique_lock locker(*this);
        CTRACE(event, "glib runloop context wakeup");
        g_main_context_wakeup(context);
        postEventSource->wakeUp();
    }

};

class GlibRunLoop : public GlibRunLoopImpl<GlibRunLoop, RunLoop>
{
    using BaseClass = GlibRunLoopImpl<GlibRunLoop, RunLoop>;
public:
    explicit GlibRunLoop();
    explicit GlibRunLoop(GMainContext& mainContext);
    ~GlibRunLoop() final;

    // From RunLoopBase
    void stopRunLoop() final;

    // from RunLoop
    void execute(ProcessFlags flags) final;

    GMainLoop* evLoop = nullptr;
};

class GlibRunLoopHook : public GlibRunLoopImpl<GlibRunLoopHook, RunLoopHook>
{
    using BaseClass = GlibRunLoopImpl<GlibRunLoopHook, RunLoopHook>;
public:
    explicit GlibRunLoopHook();
    ~GlibRunLoopHook() final;

protected:
    void stopRunLoop() final;
};

}

#endif
