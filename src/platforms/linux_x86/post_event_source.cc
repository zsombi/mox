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

gboolean GlibRunLoopBase::PostEventSource::prepare(GSource* src, gint* timeout)
{
    auto source = static_cast<PostEventSource*>(src);

    // if (source->getStatus() == RunLoop::Exiting)
    // {
    //     return false;
    // }

    bool readyToDispatch = source->m_wakeUpCalled.load();
    // If there's no event posted, wait for a second to poll again.
    if (timeout)
    {
        *timeout = 0;
    }

    CTRACE(event, "postevent source ready " << readyToDispatch);

    return readyToDispatch;
}

gboolean GlibRunLoopBase::PostEventSource::dispatch(GSource* src, GSourceFunc, gpointer)
{
    auto source = static_cast<PostEventSource*>(src);
    source->m_wakeUpCalled.store(false);

    source->m_runLoop->dispatchEvents();

    return true;
}

GlibRunLoopBase::PostEventSource* GlibRunLoopBase::PostEventSource::create(GlibRunLoopBase* loop)
{
    static GSourceFuncs funcs =
    {
        PostEventSource::prepare,
        nullptr,
        PostEventSource::dispatch,
        nullptr,
        nullptr,
        nullptr
    };

    auto self = reinterpret_cast<PostEventSource*>(g_source_new(&funcs, sizeof(PostEventSource)));
    self->m_runLoop = loop;
    self->m_wakeUpCalled = false;

    auto src = static_cast<GSource*>(self);
    // g_source_set_can_recurse(src, true);
    g_source_attach(src, loop->context);

    return self;
}

void GlibRunLoopBase::PostEventSource::destroy(PostEventSource*& source)
{
    auto src = static_cast<GSource*>(source);
    g_source_destroy(src);
    g_source_unref(src);
    source = nullptr;
}

}
