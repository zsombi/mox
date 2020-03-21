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
 * GTimerSource::Source
 */
static GSourceFuncs glibTimerSourceFuncs =
{
    GTimerSource::Source::prepare,
    nullptr,
    GTimerSource::Source::dispatch,
    nullptr,
    nullptr,
    nullptr
};

gboolean GTimerSource::Source::prepare(GSource *src, gint *timeout)
{
    Source *source = reinterpret_cast<Source*>(src);
    if (!source || !source->timer)
    {
        // Not yet ready for dispatch.
        return false;
    }

    std::chrono::duration<double> duration = std::chrono::system_clock::now() - source->lastUpdateTime;
    std::chrono::milliseconds nextHitInMsec = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    long nextTimeout = source->timer->getInterval().count() - nextHitInMsec.count();
    if (nextTimeout < 0)
    {
        nextTimeout = 0;
    }
    *timeout = (nextTimeout > G_MAXINT)
            ? G_MAXINT
            : static_cast<gint>(nextTimeout);
    CTRACE(platform, "Timer " << source->timer->id() << " to kick in " << nextTimeout << " msecs");

    return (nextTimeout == 0);
}

gboolean GTimerSource::Source::dispatch(GSource *src, GSourceFunc, gpointer)
{
    Source *source = reinterpret_cast<Source*>(src);
    if (!source || !source->timer || !source->active)
    {
        return true;
    }

    // Trigger the timer signal. Hold the timer object so it is not deleted!
    CTRACE(platform, "Timer " << source->timer->id() << " kicked");

    if (!source->timer->isSingleShot())
    {
        // Refresh the update time before we signal the Mox event source.
        source->lastUpdateTime = Timer::TimerClass::now();
    }
    else
    {
        // Deactivate the timer source, to avoid re-emission.
        source->active = false;
    }
    source->timer->signal();

    return true;
}

GTimerSource::Source* GTimerSource::Source::create(TimerRecord& timer)
{
    Source *src = reinterpret_cast<Source*>(g_source_new(&glibTimerSourceFuncs, sizeof(*src)));
    src->timer = timer.shared_from_this();
    src->lastUpdateTime = Timer::TimerClass::now();
    src->active = true;

//    g_source_set_can_recurse(src, true);

    return src;
}

void GTimerSource::Source::destroy(Source*& src)
{
    src->timer.reset();
    g_source_destroy(static_cast<GSource*>(src));
    g_source_unref(static_cast<GSource*>(src));
    src = nullptr;
}

/******************************************************************************
 * GTimerSource
 */

GTimerSource::GTimerSource(std::string_view name)
    : TimerSource(name)
{
}
GTimerSource::~GTimerSource()
{
}

void GTimerSource::addTimer(TimerRecord& timer)
{
    // Make sure the timer is registered once.
    auto finder = [tmr = timer.shared_from_this()](const Source* source)
    {
        return source->timer == tmr;
    };
    Source* gtimer = Source::create(timer);
    if (!timers.push_back_if(gtimer, finder))
    {
        Source::destroy(gtimer);
        CWARN(platform, "The timer is already registered");
        return;
    }

    GlibEventDispatcher* evLoop = static_cast<GlibEventDispatcher*>(m_runLoop.lock().get());
    g_source_attach(static_cast<GSource*>(gtimer), evLoop->context);
}

void GTimerSource::removeTimer(TimerRecord& timer)
{
    auto eraser = [tmr = timer.shared_from_this()](Source* source)
    {
        if (source->timer == tmr)
        {
            Source::destroy(source);
            return true;
        }
        return false;
    };
    erase_if(timers, eraser);
}

size_t GTimerSource::timerCount() const
{
    return timers.size();
}

void GTimerSource::clean()
{
    // Stop running timers.
    auto cleanup = [](Source* source)
    {
        if (source && source->timer)
        {
            source->timer->stop();
        }
    };
    for_each(timers, cleanup);
}

/******************************************************************************
 * TimerSource factory
 */

TimerSourcePtr Adaptation::createTimerSource(std::string_view name)
{
    return TimerSourcePtr(new GTimerSource(name));
}

}
