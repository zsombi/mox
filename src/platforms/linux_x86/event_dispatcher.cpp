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

/******************************************************************************
 * GlibEventDispatcher
 */
// Constructor for threads
GlibEventDispatcher::GlibEventDispatcher(ThreadData& threadData)
    : EventDispatcher(threadData)
{
    context = g_main_context_get_thread_default();
    FATAL(!context, "There should not be any main context at this point!!!")
    context = g_main_context_new();

    initialize();
}

// constructor for main loop
GlibEventDispatcher::GlibEventDispatcher(ThreadData& threadData, GMainContext& mainContext)
    : EventDispatcher(threadData)
    , context(&mainContext)
{
    g_main_context_ref(context);
    initialize();
}

void GlibEventDispatcher::initialize()
{
    g_main_context_push_thread_default(context);

    evLoop = g_main_loop_new(context, false);
}

GlibEventDispatcher::~GlibEventDispatcher()
{
    // Kill all the sources before the context is destroyed
    m_eventSources.clear();

    g_main_loop_unref(evLoop);
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
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

bool GlibEventDispatcher::isProcessingEvents() const
{
    return isRunning; //g_main_loop_is_running(evLoop) == TRUE;
}

void GlibEventDispatcher::processEvents(ProcessFlags flags)
{
    if (flags == ProcessFlags::RunOnce)
    {
        FlagScope<true> toggle(isRunning);
        g_main_context_iteration(context, TRUE);
    }
    else
    {
        FlagScope<true> toggle(isRunning);
        g_main_loop_run(evLoop);
    }
    forEachSource<AbstractEventSource>(&AbstractEventSource::shutDown);
}

void GlibEventDispatcher::stop()
{
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
EventDispatcherSharedPtr Adaptation::createEventDispatcher(ThreadData& threadData, bool main)
{
    if (!main)
    {
        // For main loop
        return std::make_shared<GlibEventDispatcher>(threadData, *g_main_context_default());
    }
    else
    {
        // For threads...
        return std::make_shared<GlibEventDispatcher>(threadData);
    }
}

}
