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
 * GlibRunLoopBase
 */
GlibRunLoopBase::GlibRunLoopBase(GMainContext* mainContext)
{
    context = mainContext;
    if (!context)
    {
        context = g_main_context_get_thread_default();
        FATAL(!context, "There should not be any main context at this point!!!");
        context = g_main_context_new();
        CTRACE(event, "runloop for main");
    }
    else
    {
        g_main_context_ref(context);
        CTRACE(event, "runloop for thread");
    }

    postEventSource = PostEventSource::create(this);
    socketNotifierSource = SocketNotifierSource::create(*this, context);
}

GlibRunLoopBase::~GlibRunLoopBase()
{
    auto deleteTimers = [](auto& timer)
    {
        if (!timer || !timer->m_timer)
        {
            return;
        }
        timer->m_timer->stop();
    };
    for_each(timerSources, deleteTimers);

    SocketNotifierSource::destroy(socketNotifierSource);
    PostEventSource::destroy(postEventSource);

    g_main_context_unref(context);
    CTRACE(event, "runloop down");
}
/******************************************************************************
 * GlibRunLoop
 */
// Constructor for the main loop
GlibRunLoop::GlibRunLoop()
    : BaseClass(nullptr)
{
    evLoop = g_main_loop_new(context, false);
}

// constructor for the threads
GlibRunLoop::GlibRunLoop(GMainContext& mainContext)
    : BaseClass(&mainContext)
{
    evLoop = g_main_loop_new(context, false);
}

GlibRunLoop::~GlibRunLoop()
{
    CTRACE(event, "closing glib runloop");
    g_main_loop_unref(evLoop);
}

void GlibRunLoop::execute(ProcessFlags flags)
{
    m_status = Status::Running;
    if (flags != ProcessFlags::SingleLoop)
    {
        g_main_loop_run(evLoop);
    }

    // CTRACE(event, "Final context iteration");
    // // run one more non-blocking context loop round
    // g_main_context_iteration(context, false);

    // Idle-function based shutdown may not always kick. Make sure we do.
    CTRACE(event, "notify close");
    notifyRunLoopDown();
}

void GlibRunLoop::stopRunLoop()
{
    std::unique_lock locker(*this);
    // Stop the loop;
    CTRACE(event, "glib runloop stop");
    g_main_loop_quit(evLoop);
    m_status = Status::Exiting;
}

/******************************************************************************
 * GlibRunLoopHook
 */
GlibRunLoopHook::GlibRunLoopHook()
    : BaseClass([]() -> GMainContext*
    { 
        auto ctxt = g_main_context_get_thread_default();
        if (!ctxt)
        {
            ctxt = g_main_context_default();
        }
        FATAL(ctxt, "No context to attach!");
        return ctxt;
    }())
{
    m_status = Status::Running;
}

GlibRunLoopHook::~GlibRunLoopHook()
{
}

void GlibRunLoopHook::stopRunLoop()
{
    m_status = Status::Exiting;
    notifyRunLoopDown();
}

/******************************************************************************
 * EventDispatcher factory function
 */
RunLoopPtr Adaptation::createRunLoop(bool main)
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
