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
#include <mox/utils/globals.hpp>

namespace mox
{

static EventDispatcher* mainEventLoop = nullptr;

/******************************************************************************
 * GlibEventDispatcher
 */
// Constructor for threads
GlibEventDispatcher::GlibEventDispatcher()
{
    context = g_main_context_get_thread_default();
    if (!context)
    {
        context = g_main_context_new();
    }

    initialize();
}

// constructor for main loop
GlibEventDispatcher::GlibEventDispatcher(GMainContext& mainContext)
    : context(&mainContext)
{
    initialize();
}

void GlibEventDispatcher::initialize()
{
    g_main_context_ref(context);
    g_main_context_push_thread_default(context);

    evLoop = g_main_loop_new(context, false);
}

GlibEventDispatcher::~GlibEventDispatcher()
{
    if (mainEventLoop == this)
    {
        mainEventLoop = nullptr;
    }

    // Kill all the sources before the context is destroyed
    m_eventSources.clear();

    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
    g_main_loop_unref(evLoop);
}

gboolean GlibEventDispatcher::idleFunc(gpointer userData)
{
    auto eventDispatcher = reinterpret_cast<GlibEventDispatcher*>(userData);
    if (eventDispatcher)
    {
        eventDispatcher->runIdleTasks();
        TRACE("Idle func count left: " << eventDispatcher->m_idleTasks.size());
        return eventDispatcher->m_idleTasks.size() > 0u;
    }
    return false;
}

void GlibEventDispatcher::scheduleIdleTasks()
{
    GSource *idleSource = g_idle_source_new();
    g_source_set_callback(idleSource, &GlibEventDispatcher::idleFunc, this, nullptr);
    g_source_attach(idleSource, context);
    g_source_unref(idleSource);
}

int GlibEventDispatcher::processEvents(ProcessFlags flags)
{
    setState(EventDispatchState::Running);

    if (flags == ProcessFlags::RunOnce)
    {
        bool ret = g_main_context_iteration(context, TRUE);
        if (getState() == EventDispatchState::Exiting)
        {
            forEachSource<AbstractEventSource>(&AbstractEventSource::shutDown);
        }
        if (!ret)
        {
            m_exitCode.store(std::numeric_limits<int>::min());
        }
    }
    else
    {
        g_main_loop_run(evLoop);

        forEachSource<AbstractEventSource>(&AbstractEventSource::shutDown);
        setState(EventDispatchState::Stopped);
    }

    return m_exitCode.load();
}

void GlibEventDispatcher::exit(int exitCode)
{
    setState(EventDispatchState::Exiting);
    m_exitCode.store(exitCode);
    // Stop the loop;
    g_main_loop_quit(evLoop);
}

void GlibEventDispatcher::wakeUp()
{
    forEachSource<PostEventSource>(&PostEventSource::wakeUp);
    g_main_context_wakeup(context);
}

size_t GlibEventDispatcher::runningTimerCount() const
{
    size_t count = 0;
    auto counter = [&count](TimerSourcePtr source)
    {
        count += source->timerCount();
    };
    forEachSource<TimerSource>(counter);
    return count;
}

/******************************************************************************
 * EventDispatcher factory function
 */
EventDispatcherSharedPtr Adaptation::createEventDispatcher()
{
    EventDispatcherSharedPtr evLoop;
    if (!mainEventLoop)
    {
        // For main loop
        evLoop = std::make_shared<GlibEventDispatcher>(*g_main_context_default());
        mainEventLoop = evLoop.get();
    }
    else
    {
        // For threads...
        evLoop = std::make_shared<GlibEventDispatcher>();
    }

    return evLoop;
}

}
