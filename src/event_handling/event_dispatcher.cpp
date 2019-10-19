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
namespace
{

#if defined(MOX_SINGLE_THREADED)
static EventDispatcher* tlsEventDispatcher = nullptr;
#else
static __thread EventDispatcher* tlsEventDispatcher = nullptr;
#endif

}

/******************************************************************************
 *
 */
EventDispatchState EventDispatcher::getState() const
{
    return m_eventLoopStack.empty()
            ? EventDispatchState::Inactive
            : getEventLoop()->state();
}

void EventDispatcher::setState(EventDispatchState newState)
{
    EventLoopPtr loop = getEventLoop();
    if (loop)
    {
        loop->setState(newState);
    }
}

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

EventDispatcher::EventDispatcher()
    : m_exitCode(0)
{
    tlsEventDispatcher = this;
}

EventDispatcher::~EventDispatcher()
{
    if (tlsEventDispatcher == this)
    {
        tlsEventDispatcher = nullptr;
    }
    TRACE("EventDispatcher died")
}

void EventDispatcher::pushEventLoop(EventLoop& loop)
{
    m_eventLoopStack.push(&loop);
    TRACE("Push new event loop, count now is " << m_eventLoopStack.size())
}

void EventDispatcher::popEventLoop()
{
    m_eventLoopStack.pop();
    TRACE("Popped last event loop, count now is " << m_eventLoopStack.size())
}

EventLoopPtr EventDispatcher::getEventLoop() const
{
    return m_eventLoopStack.empty() ? nullptr : m_eventLoopStack.top();
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

EventDispatcherSharedPtr EventDispatcher::create()
{
    EventDispatcherSharedPtr evLoop = Adaptation::createEventDispatcher();

    TimerSourcePtr timerSource = Adaptation::createTimerSource("default_timer");
    evLoop->addEventSource(timerSource);

    PostEventSourcePtr eventSource = Adaptation::createPostEventSource("default_post_event");
    evLoop->addEventSource(eventSource);

    SocketNotifierSourcePtr socketSource = Adaptation::createSocketNotifierSource("default_socket_notifier");
    evLoop->addEventSource(socketSource);

    return evLoop;
}

EventDispatcherSharedPtr EventDispatcher::get()
{
    return tlsEventDispatcher ? tlsEventDispatcher->shared_from_this() : nullptr;
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

bool EventDispatcher::postEvent(EventPtr&& event)
{
    EventDispatcherSharedPtr dispatcher = EventDispatcher::get();
    if (!dispatcher || dispatcher->m_eventLoopStack.empty())
    {
        return false;
    }

    if (!dispatcher->getActivePostEventSource())
    {
        return false;
    }

    if (dispatcher->getEventLoop()->postEvent(std::forward<EventPtr>(event)))
    {
        dispatcher->wakeUp();
        return true;
    }
    return false;
}

int EventDispatcher::exitCode() const
{
    return m_exitCode.load();
}

}
