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

class GPostEventSource : public EventSource
{
public:
    explicit GPostEventSource(std::string_view name);
    ~GPostEventSource() final;

    void initialize(void* data) final;

    void wakeUp() final;

    void detachOverride() final;

    struct Source : GSource
    {
        std::weak_ptr<GPostEventSource> eventSource;

        static gboolean prepare(GSource* src, gint *timeout);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);
        static void finalize(GSource*);

        static Source* create(GPostEventSource& eventSource, GMainContext* context);
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
        std::weak_ptr<GSocketNotifierSource> self;

        static gboolean prepare(GSource* src, gint *timeout);
        static gboolean check(GSource* source);
        static gboolean dispatch(GSource* source, GSourceFunc, gpointer);

        static Source* create(GSocketNotifierSource& socketSource, GMainContext* context);
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

    void initialize(void* data) final;
    void detachOverride() final;
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
    void initialize(void* data) final;
    void detachOverride() final;

    struct ZeroTimer
    {
        bool operator()(Source* const& source) const
        {
            return !source || source->timer == nullptr;
        }
    };

    SharedVector<Source*, ZeroTimer> timers;
    GMainContext* context = nullptr;
};

class GIdleSource : public IdleSource
{
    struct TaskRec
    {
        std::weak_ptr<GIdleSource> self;
        GSource* source = nullptr;
        Task task;
        TaskRec(std::shared_ptr<GIdleSource> self, GSource* source, Task&& task)
            : self(self)
            , source(source)
            , task(std::forward<Task>(task))
        {
        }
        ~TaskRec();
    };
    using TaskRecPtr = std::unique_ptr<TaskRec>;

    SharedVector<TaskRecPtr> tasks;
    GMainContext* context = nullptr;

    static gboolean sourceFunc(gpointer userData);
    static void sourceDestroy(gpointer userData);

public:
    explicit GIdleSource();
    ~GIdleSource() final;

    void wakeUp() final;
    void initialize(void* data) final;
    void detachOverride() final;

    void removeTaskRec(TaskRec& record);

protected:
    void addIdleTaskOverride(Task&& task) override;
};

class GlibRunLoop : public RunLoop
{
public:
    explicit GlibRunLoop();
    explicit GlibRunLoop(GMainContext& mainContext);
    ~GlibRunLoop() final;

    void initialize() final;

    // From RunLoopBase
    bool isRunningOverride() const final;
    void scheduleSourcesOverride() final;
    void stopRunLoop() final;

    // from RunLoop
    void execute(ProcessFlags flags) final;

    GMainLoop* evLoop = nullptr;
    GMainContext* context = nullptr;
};

class GlibRunLoopHook : public RunLoopHook
{
public:
    explicit GlibRunLoopHook();
    ~GlibRunLoopHook() final;

protected:
    void initialize() final;
    bool isRunningOverride() const final
    {
        return running;
    }
    void scheduleSourcesOverride() final;
    void stopRunLoop() final;

    GMainContext* context = nullptr;
    std::atomic_bool running = false;

};

}

#endif
