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

namespace mox
{

/******************************************************************************
 * GlibRunLoop
 */
// Constructor for the main loop
GlibRunLoop::GlibRunLoop()
{
    context = g_main_context_get_thread_default();
    FATAL(!context, "There should not be any main context at this point!!!");
    context = g_main_context_new();
}

// constructor for the threads
GlibRunLoop::GlibRunLoop(GMainContext& mainContext)
    : context(&mainContext)
{
    g_main_context_ref(context);
}

void GlibRunLoop::initialize()
{
    forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::initialize, context);
    evLoop = g_main_loop_new(context, false);
}

GlibRunLoop::~GlibRunLoop()
{
    CTRACE(event, "closing glib runloop");
    g_main_loop_unref(evLoop);

    g_main_context_unref(context);
    CTRACE(event, "runloop down");
}

bool GlibRunLoop::isRunningOverride() const
{
    return g_main_loop_is_running(evLoop) == TRUE;
}

void GlibRunLoop::execute(ProcessFlags flags)
{
    if (flags != ProcessFlags::SingleLoop)
    {
        g_main_loop_run(evLoop);
    }

    CTRACE(event, "Final context iteration");
    // run one more non-blocking context loop round
    g_main_context_iteration(context, false);

    // Idle-function based shutdown may not always kick. Make sure we do.
    CTRACE(event, "notify close");
    notifyRunLoopDown();
}

void GlibRunLoop::stopRunLoop()
{
    // Stop the loop;
    CTRACE(event, "glib runloop stop");
    g_main_loop_quit(evLoop);
}

void GlibRunLoop::scheduleSourcesOverride()
{
    CTRACE(event, "glib runloop context wakeup");
    g_main_context_wakeup(context);
}

/******************************************************************************
 * GlibRunLoopHook
 */
GlibRunLoopHook::GlibRunLoopHook()
{
    context = g_main_context_get_thread_default();
    if (!context)
    {
        context = g_main_context_default();
    }
    FATAL(context, "No context to attach");
    g_main_context_ref(context);
}

GlibRunLoopHook::~GlibRunLoopHook()
{
    g_main_context_unref(context);
    CTRACE(event, "runloop hook down");
}

void GlibRunLoopHook::initialize()
{
    forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::initialize, context);
    running = true;
}

void GlibRunLoopHook::scheduleSourcesOverride()
{
    CTRACE(event, "glib runloophook context wakeup");
    g_main_context_wakeup(context);
}

void GlibRunLoopHook::stopRunLoop()
{
    running = false;
    notifyRunLoopDown();
}

/******************************************************************************
 * EventDispatcher factory function
 */
RunLoopSharedPtr Adaptation::createRunLoop(bool main)
{
    auto runLoop = std::shared_ptr<GlibRunLoop>();
    if (main)
    {
        // For the main loop
        runLoop = make_polymorphic_shared<RunLoop, GlibRunLoop>();
    }
    else
    {
        // For threads
        runLoop = make_polymorphic_shared<RunLoop, GlibRunLoop>(*g_main_context_default());
    }
    return runLoop;
}

RunLoopHookPtr Adaptation::createRunLoopHook()
{
    return make_polymorphic_shared<RunLoopHook, GlibRunLoopHook>();
}

}
