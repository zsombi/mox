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

#include <mox/core/event_handling/event.hpp>
#include <mox/core/event_handling/event_queue.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/object.hpp>
#include <mox/core/timer.hpp>
#include <mox/core/platforms/adaptation.hpp>

#include <utility>

namespace mox
{

/******************************************************************************
 * RunLoopBase
 */
void RunLoopBase::setupSources()
{
    AbstractRunLoopSourceSharedPtr source;
    source = Adaptation::createPostEventSource("default_post_event");
    source->attach(*this);

    source = Adaptation::createTimerSource("default_timer");
    source->attach(*this);

    source = Adaptation::createSocketNotifierSource("default_socket_notifier");
    source->attach(*this);
}

void RunLoopBase::addSource(AbstractRunLoopSourceSharedPtr source)
{
    m_runLoopSources.push_back(source);
}

void RunLoopBase::removeSource(AbstractRunLoopSource& source)
{
    auto finder = [&source](auto& src)
    {
        return &source == src.get();
    };
    auto it = std::find_if(m_runLoopSources.begin(), m_runLoopSources.end(), finder);
    if (it != m_runLoopSources.end())
    {
        (*it).reset();
    }
}

void RunLoopBase::notifyRunLoopDown()
{
    forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::detach);
    if (m_closedCallback)
    {
        m_closedCallback();
    }
}


void RunLoopBase::setRunLoopDownCallback(DownCallback callback)
{
    m_closedCallback = callback;
}

AbstractRunLoopSourceSharedPtr RunLoopBase::findSource(std::string_view name)
{
    for (auto source : m_runLoopSources)
    {
        if (source->name() == name)
        {
            return source;
        }
    }

    return nullptr;
}

AbstractRunLoopSourceSharedPtr RunLoopBase::findSource(std::string_view name) const
{
    for (auto source : m_runLoopSources)
    {
        if (source->name() == name)
        {
            return source;
        }
    }

    return nullptr;
}

TimerSourcePtr RunLoopBase::getDefaultTimerSource()
{
    if (isExiting())
    {
        return nullptr;
    }
    // the first source is the timer source.
    auto source = std::static_pointer_cast<TimerSource>(m_runLoopSources[1]);
    return source && source->isFunctional() ? source : nullptr;
}

EventSourcePtr RunLoopBase::getDefaultPostEventSource()
{
    if (isExiting())
    {
        return nullptr;
    }
    // the secont source is the event source.
    auto source = std::static_pointer_cast<EventSource>(m_runLoopSources[0]);
    return source && source->isFunctional() ? source : nullptr;
}

SocketNotifierSourcePtr RunLoopBase::getDefaultSocketNotifierSource()
{
    if (isExiting())
    {
        return nullptr;
    }
    // the third source is the socket notifier source.
    auto source = std::static_pointer_cast<SocketNotifierSource>(m_runLoopSources[2]);
    return source && source->isFunctional() ? source : nullptr;
}

void RunLoopBase::scheduleSources()
{
    forEachSource<AbstractRunLoopSource>(&AbstractRunLoopSource::wakeUp);
    scheduleSourcesOverride();
}

void RunLoopBase::quit()
{
    if (m_isExiting)
    {
        CTRACE(event, "The runloop is already exiting.");
//        return;
    }

    stopRunLoop();

    m_isExiting = true;
}

bool RunLoopBase::isExiting() const
{
    return m_isExiting;
}

bool RunLoopBase::isRunning() const
{
    return isRunningOverride();
}

void RunLoopBase::onIdle(IdleFunction idle)
{
    if (isExiting())
    {
        return;
    }
    onIdleOverride(std::move(idle));
}

/******************************************************************************
 * RunLoop
 */
RunLoop::~RunLoop()
{
    CTRACE(event, "RunLoop died");
}

RunLoopSharedPtr RunLoop::create(bool main)
{
    auto evLoop = Adaptation::createRunLoop(main);

    evLoop->setupSources();
    evLoop->initialize();

    return evLoop;
}

/******************************************************************************
 * RunLoopHook
 */

RunLoopHookPtr RunLoopHook::create()
{
    auto evLoop = Adaptation::createRunLoopHook();

    evLoop->setupSources();
    evLoop->initialize();

    return evLoop;
}

}
