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

#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_handler.hpp>

namespace mox
{

/******************************************************************************
 *
 */
class HandlerToken : public EventHandlingProvider::Token
{
    EventHandlingProvider::EventHandler m_handler;
public:
    explicit HandlerToken(const EventHandlingProvider::EventHandler& handler)
        : m_handler(handler)
    {
    }

    EventHandlingProvider::EventHandler handler() const
    {
        return m_handler;
    }
};

/******************************************************************************
 *
 */
class FilterToken : public EventHandlingProvider::Token
{
    EventHandlingProvider::EventFilter m_filter;
public:
    explicit FilterToken(const EventHandlingProvider::EventFilter& filter)
        : m_filter(filter)
    {
    }

    EventHandlingProvider::EventFilter filter() const
    {
        return m_filter;
    }
};

/******************************************************************************
 *
 */
EventHandlingProvider::EventHandlingProvider()
{
    auto deferredSignalHandler = [](Event& event)
    {
        DeferredSignalEvent& deferredSignal = dynamic_cast<DeferredSignalEvent&>(event);
        deferredSignal.activate();
    };
    addEventHandler(EventType::DeferredSignal, deferredSignalHandler);
}

EventHandlingProvider::~EventHandlingProvider()
{
}

EventHandlingProvider::TokenPtr EventHandlingProvider::addEventHandler(EventType type, const EventHandler &handler)
{
    TokenPtr token = make_polymorphic_shared<Token, HandlerToken>(handler);

    auto it = m_handlers.find(type);
    if (it == m_handlers.end())
    {
        TokenList list;
        list.push_back(token);
        m_handlers.insert(make_pair(type, std::forward<TokenList>(list)));
    }
    else
    {
        it->second.push_back(token);
    }
    return token;
}

EventHandlingProvider::TokenPtr EventHandlingProvider::addEventFilter(EventType type, const EventFilter &filter)
{
    TokenPtr token = make_polymorphic_shared<Token, FilterToken>(filter);

    auto it = m_filters.find(type);
    if (it == m_filters.end())
    {
        TokenList list;
        list.push_back(token);
        m_filters.insert(make_pair(type, std::forward<TokenList>(list)));
    }
    else
    {
        it->second.push_back(token);
    }
    return token;
}

bool EventHandlingProvider::filterEvent(Event& event)
{
    auto filter = m_filters.find(event.type());
    if (filter == m_filters.end())
    {
        return false;
    }

    for (auto& token : filter->second)
    {
        FilterToken* filterToken = static_cast<FilterToken*>(token.get());

        // Mark the event consumed.
        event.setHandled(true);
        if (filterToken->filter()(event))
        {
            return true;
        }
        event.setHandled(false);
    }
    return false;
}

void EventHandlingProvider::processEvent(Event& event)
{
    auto handlerList = m_handlers.find(event.type());
    if (handlerList == m_handlers.end())
    {
        return;
    }

    for (auto& token : handlerList->second)
    {
        HandlerToken* handler = static_cast<HandlerToken*>(token.get());

        // Mark the event consumed.
        event.setHandled(true);
        handler->handler()(event);
        if (event.isHandled())
        {
            return;
        }
    }
}

}
