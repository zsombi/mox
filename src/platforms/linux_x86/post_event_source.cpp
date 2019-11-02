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
 * GPostEventSource::Source
 */
gboolean GPostEventSource::Source::prepare(GSource* src, gint *timeout)
{
    Source* source = reinterpret_cast<Source*>(src);

    bool readyToDispatch = source->eventSource->wakeUpCalled.load() && !source->eventSource->m_eventQueue.empty();
    // If there's no event posted, wait for a second to poll again.
    *timeout = readyToDispatch ? -1 : 5000;

    TRACE("posted event source ready " << readyToDispatch);

    return readyToDispatch;
}

gboolean GPostEventSource::Source::dispatch(GSource* src, GSourceFunc, gpointer)
{
    Source* source = reinterpret_cast<Source*>(src);

    source->eventSource->wakeUpCalled.store(false);
    // Process the event in the loop.
    source->eventSource->dispatch();
    // Keep it rolling.
    return true;
}

static GSourceFuncs postEventSourceFuncs =
{
    GPostEventSource::Source::prepare,
    nullptr,
    GPostEventSource::Source::dispatch,
    nullptr,
    nullptr,
    nullptr
};

GPostEventSource::Source* GPostEventSource::Source::create(GPostEventSource& eventSource)
{
    Source* source = reinterpret_cast<Source*>(g_source_new(&postEventSourceFuncs, sizeof(*source)));
    source->eventSource = &eventSource;
    source->eventSource->wakeUpCalled.store(false);

    GSource* src = static_cast<GSource*>(source);
//    g_source_set_can_recurse(src, true);
    GlibEventDispatcher* loop = static_cast<GlibEventDispatcher*>(source->eventSource->eventDispatcher().get());
    g_source_attach(src, loop->context);

    return source;
}

void GPostEventSource::Source::destroy(Source*& source)
{
    GSource* gsource = static_cast<GSource*>(source);
    if (!gsource)
    {
        return;
    }
    g_source_destroy(gsource);
    g_source_unref(gsource);
    source = nullptr;
}

/******************************************************************************
 * GPostEventSource
 */

GPostEventSource::GPostEventSource(std::string_view name)
    : PostEventSource(name)
    , source(nullptr)
{
}
GPostEventSource::~GPostEventSource()
{
    Source::destroy(source);
}

void GPostEventSource::prepare()
{
    source = Source::create(*this);
}

void GPostEventSource::wakeUp()
{
    wakeUpCalled.store(true);
}

/******************************************************************************
 * PostEventSource factory function
 */
PostEventSourcePtr Adaptation::createPostEventSource(std::string_view name)
{
    return PostEventSourcePtr(new GPostEventSource(name));
}

}
