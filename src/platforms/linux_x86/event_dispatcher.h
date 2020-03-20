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

#include <mox/meta/class/metaobject.hpp>
#include <mox/event_handling/run_loop.hpp>
#include <mox/event_handling/run_loop_sources.hpp>
#include <mox/timer.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <mox/platforms/adaptation.hpp>

#include <glib.h>

namespace mox
{

class GlibEventDispatcher;

class GPostEventSource : public EventSource
{
public:
    explicit GPostEventSource(std::string_view name);
    ~GPostEventSource() final;

    void prepare() final;

    void wakeUp() final;

    struct Source : GSource
    {
        GPostEventSource* eventSource = nullptr;

        static gboolean prepare(GSource* src, gint *timeout);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static Source* create(GPostEventSource& eventSource);
        static void destroy(Source*& source);
    };

    Source* source = nullptr;
    atomic<bool> wakeUpCalled;
};

struct GPollHandler
{
    GPollFD fd;
    SocketNotifierSource::NotifierPtr notifier;
    explicit GPollHandler(SocketNotifierSource::Notifier& notifier);
    void reset();
};

class GSocketNotifierSource : public SocketNotifierSource
{
public:
    struct Source : GSource
    {
        GSocketNotifierSource* self = nullptr;

        static gboolean prepare(GSource* src, gint *timeout);
        static gboolean check(GSource* source);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static Source* create(GSocketNotifierSource& socketSource);
        static void destroy(Source*& source);
    };

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

    Source* source = nullptr;
    SharedVector<GPollHandler, NullPollHandler, PollInvalidator> pollHandlers;

    explicit GSocketNotifierSource(std::string_view name);
    ~GSocketNotifierSource() final;

    void prepare() final;
    void clean() final;
    void addNotifier(Notifier& notifier) final;
    void removeNotifier(Notifier& notifier) final;
};

class GTimerSource : public TimerSource
{
public:
    struct Source : GSource
    {
        TimerSource::TimerPtr timer;
        Timestamp lastUpdateTime;
        bool active = true;

        static gboolean prepare(GSource* src, gint* timeout);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static Source *create(TimerRecord& timer);
        static void destroy(Source*& src);
    };

    explicit GTimerSource(std::string_view name);
    ~GTimerSource() final;

    // From TimerSource
    void addTimer(TimerRecord& timer) final;
    void removeTimer(TimerRecord& timer) final;
    size_t timerCount() const final;

    // from AbstractRunLoopSource
    void clean() final;

    struct ZeroTimer
    {
        bool operator()(Source* const& source) const
        {
            return !source || source->timer == nullptr;
        }
    };

    SharedVector<Source*, ZeroTimer> timers;
};

class GlibEventDispatcher : public RunLoop
{
    void initialize();

    static gboolean idleFunc(gpointer userData);

public:
    explicit GlibEventDispatcher();
    explicit GlibEventDispatcher(GMainContext& mainContext);
    ~GlibEventDispatcher() override;

    bool isRunning() const override;
    void execute(ProcessFlags flags) override;
    void stopExecution() override;
    void shutDown() override;
    void wakeUp() override;
    size_t runningTimerCount() const override;

    void scheduleIdleTasks() override;

    GMainLoop* evLoop = nullptr;
    GMainContext* context = nullptr;
    bool running = false;
};

}

#endif
