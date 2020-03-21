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
 *
 */
bool RunLoop::runIdleTasks()
{
    decltype (m_idleTasks) newQueue;

    while (!m_idleTasks.empty())
    {
        RunLoop::IdleFunction idleFunction(std::move(m_idleTasks.front()));
        m_idleTasks.pop();

        if (!idleFunction())
        {
            // Shall run with next idle loop aswell.
            newQueue.push(std::move(idleFunction));
        }
    }
    if (!newQueue.empty())
    {
        m_idleTasks.swap(newQueue);
    }

    return !m_idleTasks.empty();
}

RunLoop::RunLoop()
{
}

RunLoop::~RunLoop()
{
    CTRACE(event, "RunLoop died");
}

EventSourcePtr RunLoop::getActiveEventSource()
{
    // Get the last event source. Only one event source is used to handle the queue.
    auto finder = [](AbstractRunLoopSourceSharedPtr abstractSource)
    {
        return std::dynamic_pointer_cast<EventSource>(abstractSource) != nullptr;
    };
    auto it = std::find_if(m_runLoopSources.rbegin(), m_runLoopSources.rend(), finder);
    if (it == m_runLoopSources.rend())
    {
        return nullptr;
    }
    return std::static_pointer_cast<EventSource>(*it);
}

RunLoopSharedPtr RunLoop::create(bool main)
{
    auto evLoop = Adaptation::createRunLoop(main);

    auto timerSource = Adaptation::createTimerSource("default_timer");
    evLoop->addSource(timerSource);

    auto eventSource = Adaptation::createPostEventSource("default_post_event");
    evLoop->addSource(eventSource);

    auto socketSource = Adaptation::createSocketNotifierSource("default_socket_notifier");
    evLoop->addSource(socketSource);

    return evLoop;
}

void RunLoop::addSource(AbstractRunLoopSourceSharedPtr source)
{
    for (auto evs : m_runLoopSources)
    {
        if (evs == source)
        {
            return;
        }
    }

    m_runLoopSources.push_back(source);
    source->setRunLoop(*this);
    source->prepare();
}

AbstractRunLoopSourceSharedPtr RunLoop::findSource(std::string_view name)
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

AbstractRunLoopSourceSharedPtr RunLoop::findSource(std::string_view name) const
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

TimerSourcePtr RunLoop::getDefaultTimerSource()
{
    // the first source is the timer source.
    return std::static_pointer_cast<TimerSource>(m_runLoopSources[0]);
}

EventSourcePtr RunLoop::getDefaultPostEventSource()
{
    // the secont source is the event source.
    return std::static_pointer_cast<EventSource>(m_runLoopSources[1]);
}

SocketNotifierSourcePtr RunLoop::getDefaultSocketNotifierSource()
{
    // the third source is the socket notifier source.
    return std::static_pointer_cast<SocketNotifierSource>(m_runLoopSources[2]);
}

void RunLoop::addIdleTask(IdleFunction function)
{
    m_idleTasks.push(std::move(function));
    scheduleIdleTasks();
}

}
