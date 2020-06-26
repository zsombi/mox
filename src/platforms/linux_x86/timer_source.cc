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
 * GLibRunLoopBase::TimerSource
 */
gboolean GlibRunLoopBase::TimerSource::prepare(GSource* src, gint* timeout)
{
    auto source = static_cast<TimerSource*>(src);
    if (!source || !source->m_timer)
    {
        // Not yet ready for dispatch.
        return false;
    }

    std::chrono::duration<double> duration = std::chrono::system_clock::now() - source->m_lastUpdateTime;
    std::chrono::milliseconds nextHitInMsec = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    long nextTimeout = source->m_timer->getInterval().count() - nextHitInMsec.count();
    if (nextTimeout < 0)
    {
        nextTimeout = 0;
    }
    *timeout = (nextTimeout > G_MAXINT)
            ? G_MAXINT
            : static_cast<gint>(nextTimeout);
    CTRACE(platform, "Timer " << source->m_timer->id() << " to kick in " << nextTimeout << " msecs");

    return (nextTimeout == 0);
}

gboolean GlibRunLoopBase::TimerSource::dispatch(GSource* src, GSourceFunc, gpointer)
{
    auto source = static_cast<TimerSource*>(src);
    if (!source || !source->m_timer || !source->m_active)
    {
        return true;
    }

    // Trigger the timer signal. Hold the timer object so it is not deleted!
    CTRACE(platform, "Timer " << source->m_timer->id() << " kicked");

    if (!source->m_timer->isSingleShot())
    {
        // Refresh the update time before we signal the Mox event source.
        source->m_lastUpdateTime = Timer::TimerClass::now();
    }
    else
    {
        // Deactivate the timer source, to avoid re-emission.
        source->m_active = false;
    }
    source->m_timer->signal();

    return true;
}

GlibRunLoopBase::TimerSource* GlibRunLoopBase::TimerSource::create(TimerCore& timer, GMainContext* context)
{
    static GSourceFuncs funcs =
    {
        TimerSource::prepare,
        nullptr,
        TimerSource::dispatch,
        nullptr,
        nullptr,
        nullptr
    };

    auto self = reinterpret_cast<TimerSource*>(g_source_new(&funcs, sizeof(TimerSource)));
    self->m_timer = timer.shared_from_this();
    self->m_lastUpdateTime = Timer::TimerClass::now();
    self->m_active = true;

    g_source_attach(static_cast<GSource*>(self), context);

    return self;
}

void GlibRunLoopBase::TimerSource::destroy(TimerSource*& source)
{
    auto src = static_cast<GSource*>(source);
    g_source_destroy(src);
    g_source_unref(src);
    source = nullptr;
}

}
