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
 * GPostEventSource::Source
 */
gboolean GPostEventSource::Source::prepare(GSource* src, gint *timeout)
{
    Source* source = reinterpret_cast<Source*>(src);
    auto evSource = source->eventSource.lock();
    if (!evSource)
    {
        CFATAL(event, false, "Orphan post event source invoked!" << src);
        return false;
    }

    if (!evSource->isFunctional())
    {
        CINFO(event, "the post event source of this runloop is no longer functional");
        return false;
    }
    if (evSource->getRunLoop()->isExiting())
    {
        CINFO(event, "the runloop is exiting, do not process events any further");
        return false;
    }

    bool readyToDispatch = evSource->wakeUpCalled.load() && !evSource->m_eventQueue->empty();
    // If there's no event posted, wait for a second to poll again.
    if (timeout)
        *timeout = -1;

    CTRACE(platform, "postevent source ready " << readyToDispatch);

    return readyToDispatch;
}

gboolean GPostEventSource::Source::dispatch(GSource* src, GSourceFunc, gpointer)
{
    Source* source = reinterpret_cast<Source*>(src);
    auto evSource = source->eventSource.lock();
    if (!evSource)
    {
        CFATAL(event, false, "Orphan post event source invoked!" << src);
        return false;
    }

    evSource->wakeUpCalled.store(false);
    // Process the event in the loop.
    evSource->dispatchQueuedEvents();
    // Keep it rolling.
    return true;
}

void GPostEventSource::Source::finalize(GSource* src)
{
    UNUSED(src);
    CTRACE(event, "Finalizing postevent source" << src);
}

static GSourceFuncs postEventSourceFuncs =
{
    GPostEventSource::Source::prepare,
    nullptr,
    GPostEventSource::Source::dispatch,
    GPostEventSource::Source::finalize,
    nullptr,
    nullptr
};

GPostEventSource::Source* GPostEventSource::Source::create(GPostEventSource& eventSource, GMainContext* context)
{
    Source* source = reinterpret_cast<Source*>(g_source_new(&postEventSourceFuncs, sizeof(*source)));
    auto self = as_shared<GPostEventSource>(&eventSource);
    source->eventSource = self;
    self->wakeUpCalled.store(false);

    GSource* src = static_cast<GSource*>(source);
//    g_source_set_can_recurse(src, true);
    g_source_attach(src, context);

    CTRACE(event, "post event source created:" << source);

    return source;
}

void GPostEventSource::Source::destroy(Source*& source)
{
    GSource* gsource = static_cast<GSource*>(source);
    if (!gsource)
    {
        return;
    }
    g_source_unref(gsource);
    g_source_destroy(gsource);
    CTRACE(event, "post source runloop destroyed:" << source <<"- ref_count=" << gsource->ref_count);
    source = nullptr;
}

/******************************************************************************
 * GPostEventSource
 */

GPostEventSource::GPostEventSource(std::string_view name)
    : EventSource(name)
    , source(nullptr)
{
}
GPostEventSource::~GPostEventSource()
{
    CTRACE(event, "postevent runloop source deleted");
}

void GPostEventSource::initialize(void* data)
{
    CTRACE(event, "initialize PostEvent runloop source");
    source = Source::create(*this, reinterpret_cast<GMainContext*>(data));
}

void GPostEventSource::wakeUp()
{
    wakeUpCalled.store(true);
}

void GPostEventSource::detachOverride()
{
    CTRACE(event, "detach SocketNotifier runloop source");
    Source::destroy(source);
}

/******************************************************************************
 * PostEventSource factory function
 */
EventSourcePtr Adaptation::createPostEventSource(std::string_view name)
{
    return make_polymorphic_shared<EventSource, GPostEventSource>(name);
}

}
