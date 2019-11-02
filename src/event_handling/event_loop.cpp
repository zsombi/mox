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

#include <mox/event_handling/event_loop.hpp>
#include <mox/event_handling/event_handler.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/module/thread_loop.hpp>
#include <mox/object.hpp>

namespace mox
{

namespace
{

template <typename Function>
bool dispatchToHandlers(bool tunneled, ObjectSharedPtr target, Function function, Event& event)
{
    if (!target)
    {
        return false;
    }
    Object* parent = target->parent();

    // Tunneling
    if (tunneled && parent && dispatchToHandlers(tunneled, parent->shared_from_this(), function, event))
    {
        return true;
    }

    if (function(*target, event))
    {
        return true;
    }

    // Bubbling
    if (!tunneled && parent && dispatchToHandlers(tunneled, parent->shared_from_this(), function, event))
    {
        return true;
    }
    return false;
}

}
/******************************************************************************
 *
 */
EventLoop::EventLoop()
    : m_threadData(*ThreadData::thisThreadData())
    , m_dispatcher(m_threadData.eventDispatcher())
{
    m_threadData.m_eventLoopStack.push(this);
    TRACE("Event loop added to thread data, count is " << m_threadData.m_eventLoopStack.size())
}

EventLoop::~EventLoop()
{
    m_threadData.m_eventLoopStack.pop();
    TRACE("Event loop removed from thread data, count is " << m_threadData.m_eventLoopStack.size())
}

int EventLoop::processEvents(ProcessFlags flags)
{
    if (!m_dispatcher)
    {
        throw no_event_dispatcher();
    }

    m_dispatcher->processEvents(flags);
    return m_threadData.exitCode();
}

void EventLoop::exit(int exitCode)
{
    UNUSED(exitCode);
    if (!m_dispatcher)
    {
        throw no_event_dispatcher();
    }
    postEvent<QuitEvent>(m_threadData.thread(), exitCode);
}

void EventLoop::wakeUp()
{
    if (!m_dispatcher)
    {
        throw no_event_dispatcher();
    }
    m_dispatcher->wakeUp();
}

/******************************************************************************
 *
 */
bool postEvent(EventPtr&& event)
{
    ObjectSharedPtr target = event->target();
    if (!target)
    {
        return false;
    }

    ThreadDataSharedPtr threadData = target->threadData();
    if (!threadData)
    {
        return false;
    }

    threadData->m_eventQueue.push(std::forward<EventPtr>(event));
    threadData->eventDispatcher()->wakeUp();

    return true;
}

bool sendEvent(Event& event)
{
    ObjectSharedPtr spTarget = event.target();
    if (!spTarget)
    {
        return false;
    }

    if (spTarget->threadData() != ThreadData::thisThreadData())
    {
        std::cerr << "Post the events dedicated to targets residing in a different thread" << std::endl;
        return false;
    }

    // Filter by tunneling
    auto tunnel = [](Object& handler, Event& event)
    {
        return handler.filterEvent(event);
    };
    if (dispatchToHandlers(true, spTarget, tunnel, event))
    {
        return true;
    }

    // Process by bubbling.
    auto bubble = [](Object& handler, Event& event)
    {
        handler.processEvent(event);
        return event.isHandled();
    };
    return dispatchToHandlers(false, spTarget, bubble, event);
}

}
