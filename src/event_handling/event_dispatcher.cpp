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

#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/event_loop.hpp>
#include <mox/event_handling/event_handler.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_queue.hpp>
#include <mox/event_handling/event_sources.hpp>
#include <mox/object.hpp>
#include <mox/timer.hpp>
#include <mox/platforms/adaptation.hpp>

#include <utility>

namespace mox
{

/******************************************************************************
 *
 */
bool EventDispatcher::runIdleTasks()
{
    decltype (m_idleTasks) newQueue;

    while (!m_idleTasks.empty())
    {
        EventDispatcher::IdleFunction idleFunction(std::move(m_idleTasks.front()));
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

EventDispatcher::EventDispatcher(ThreadData& threadData)
    : m_threadData(threadData)
{
}

EventDispatcher::~EventDispatcher()
{
    TRACE("EventDispatcher died")
}

PostEventSourcePtr EventDispatcher::getActivePostEventSource()
{
    // Get the last post event source. Only one post event source is used to handle the queue.
    auto finder = [](AbstractEventSourceSharedPtr abstractSource)
    {
        return std::dynamic_pointer_cast<PostEventSource>(abstractSource) != nullptr;
    };
    auto it = std::find_if(m_eventSources.rbegin(), m_eventSources.rend(), finder);
    if (it == m_eventSources.rend())
    {
        return nullptr;
    }
    return std::static_pointer_cast<PostEventSource>(*it);
}

EventDispatcherSharedPtr EventDispatcher::create(ThreadData& threadData, bool main)
{
    auto evLoop = Adaptation::createEventDispatcher(threadData, main);

    auto timerSource = Adaptation::createTimerSource("default_timer");
    evLoop->addEventSource(timerSource);

    auto eventSource = Adaptation::createPostEventSource("default_post_event");
    evLoop->addEventSource(eventSource);

    auto socketSource = Adaptation::createSocketNotifierSource("default_socket_notifier");
    evLoop->addEventSource(socketSource);

    return evLoop;
}

void EventDispatcher::addEventSource(AbstractEventSourceSharedPtr source)
{
    for (auto evs : m_eventSources)
    {
        if (evs == source)
        {
            return;
        }
    }

    m_eventSources.push_back(source);
    source->setEventDispatcher(*this);
    source->prepare();
}

AbstractEventSourceSharedPtr EventDispatcher::findEventSource(std::string_view name)
{
    for (auto source : m_eventSources)
    {
        if (source->name() == name)
        {
            return source;
        }
    }

    return nullptr;
}

AbstractEventSourceSharedPtr EventDispatcher::findEventSource(std::string_view name) const
{
    for (auto source : m_eventSources)
    {
        if (source->name() == name)
        {
            return source;
        }
    }

    return nullptr;
}

void EventDispatcher::addIdleTask(IdleFunction&& function)
{
    m_idleTasks.push(std::move(function));
    scheduleIdleTasks();
}

}
