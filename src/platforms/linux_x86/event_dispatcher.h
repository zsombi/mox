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

#include <glib.h>

namespace mox
{

class GlibEventDispatcher;

class GPostEventSource : public PostEventSource
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
    SocketNotifierSharedPtr notifier;
    explicit GPollHandler(SocketNotifier& notifier);
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

    Source* source = nullptr;
    SharedVector<GPollHandler, NullPollHandler> pollHandlers;

    explicit GSocketNotifierSource(std::string_view name);
    ~GSocketNotifierSource() final;

    void prepare() final;
    void shutDown() final;
    void addNotifier(SocketNotifier& notifier) final;
    void removeNotifier(SocketNotifier& notifier) final;
};

class GTimerSource : public TimerSource
{
public:
    struct Source : GSource
    {
        TimerPtr timer;
        Timestamp lastUpdateTime;
        bool active = true;

        static gboolean prepare(GSource* src, gint* timeout);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static Source *create(Timer& timer);
        static void destroy(Source*& src);
    };

    explicit GTimerSource(std::string_view name);
    ~GTimerSource() final;

    std::optional<size_t> findSource(TimerPtr timer);

    // From TimerSource
    void addTimer(Timer& timer) final;
    void removeTimer(Timer& timer) final;
    size_t timerCount() const final;

    // from AbstractEventSource
    void shutDown() final;

    struct ZeroTimer
    {
        bool operator()(Source* const& source) const
        {
            return !source || source->timer == nullptr;
        }
    };

    SharedVector<Source*, ZeroTimer> timers;
};

class GlibEventDispatcher : public EventDispatcher
{
    void initialize();

    static gboolean idleFunc(gpointer userData);

public:
    explicit GlibEventDispatcher(ThreadData& threadData);
    explicit GlibEventDispatcher(ThreadData& threadData, GMainContext& mainContext);
    ~GlibEventDispatcher() override;

    bool isProcessingEvents() const override;
    void processEvents(ProcessFlags flags) override;
    void stop() override;
    void wakeUp() override;
    size_t runningTimerCount() const override;

    void scheduleIdleTasks() override;

    GMainLoop* evLoop = nullptr;
    GMainContext* context = nullptr;
    bool isRunning = false;
};

}

#endif
