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
    : m_dispatcher(EventDispatcher::get())
    , m_state(EventDispatchState::Inactive)
{
    m_dispatcher->pushEventLoop(*this);
}

EventLoop::~EventLoop()
{
    m_dispatcher->popEventLoop();
}

void EventLoop::setState(EventDispatchState newState)
{
    TRACE("State changed from " << int(m_state.load()) << " to " << int(newState))
    m_state.store(newState);
}

bool EventLoop::postEvent(EventPtr&& event)
{
    EventDispatchState state = m_state.load();
    if (state == EventDispatchState::Exiting || state == EventDispatchState::Stopped)
    {
        return false;
    }
    m_queue.push(std::forward<EventPtr>(event));
    return true;
}

bool EventLoop::sendEvent(Event& event)
{
    ObjectSharedPtr spTarget = event.target();
    if (!spTarget)
    {
        return false;
    }

    // Filter by tunneling
    auto filter = [](Object& handler, Event& event)
    {
        return handler.filterEvent(event);
    };
    if (dispatchToHandlers(true, spTarget, filter, event))
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

void EventLoop::dispatchEventQueue()
{
    if (m_queue.empty())
    {
        return;
    }

    TRACE("process posted events")
    m_queue.process(sendEvent);
}

int EventLoop::processEvents(ProcessFlags flags)
{
    return m_dispatcher->processEvents(flags);
}

void EventLoop::exit(int exitCode)
{
    if (!exitCode)
    {
        exitCode = m_dispatcher->exitCode();
    }
    m_dispatcher->exit(exitCode);
}

}
