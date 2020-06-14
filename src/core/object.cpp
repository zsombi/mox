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

#include <mox/core/object.hpp>

namespace mox
{

/******************************************************************************
 * internals
 */
namespace
{

/******************************************************************************
 *
 */
class HandlerToken : public Object::EventToken
{
    Object::EventHandler m_handler;

public:
    explicit HandlerToken(EventType type, ObjectSharedPtr eventHandler, Object::EventHandler&& handler)
        : EventToken(type, eventHandler)
        , m_handler(handler)
    {
    }

    auto handler() const
    {
        return m_handler;
    }
};

/******************************************************************************
 *
 */
class FilterToken : public Object::EventToken
{
    Object::EventFilter m_filter;
public:
    explicit FilterToken(EventType type, ObjectSharedPtr eventHandler, Object::EventFilter&& filter)
        : EventToken(type, eventHandler)
        , m_filter(filter)
    {
    }

    auto filter() const
    {
        return m_filter;
    }
};

}

/******************************************************************************
 * Object::EventToken
 */
Object::EventToken::EventToken(EventType type, ObjectSharedPtr target)
    : m_target(target)
    , m_type(type)
{
}

void Object::EventToken::erase()
{
    auto target = m_target.lock();
    if (!target)
    {
        return;
    }

    auto filter = dynamic_cast<FilterToken*>(this);
    if (filter)
    {
        auto it = target->m_filters.find(m_type);
        mox::erase(it->second, shared_from_this());
    }
    else
    {
        auto it = target->m_handlers.find(m_type);
        mox::erase(it->second, shared_from_this());
    }
    m_target.reset();
}

bool Object::EventToken::isValid() const
{
    return !m_target.expired();
}

/******************************************************************************
 * Object
 */
Object::Object()
    : m_threadData(ThreadData::getThisThreadData())
{
}

void Object::removeChildren()
{
    lock_guard lock(*this);

    auto remover = [this](auto& child)
    {
        if (!child)
        {
            return;
        }
        ScopeRelock relock(*this);
        this->removeChild(*child);
    };
    for_each(m_children, remover);
}

Object::~Object()
{
    if (m_parent)
    {
        m_parent->removeChild(*this);
    }

    removeChildren();

    m_threadData.reset();
}

ObjectSharedPtr Object::create(Object* parent)
{
    return createObject(new Object, parent);
}

Object* Object::getParent() const
{
    return m_parent;
}

Object::VisitResult Object::moveToThread(ThreadDataSharedPtr threadData)
{
    m_threadData = threadData;
    return VisitResult::Continue;
}


Object::EventDispatcher::EventDispatcher(Object& target)
{
    auto td = target.threadData();
    for (auto parent = &target; parent && (td == parent->threadData()); parent = parent->m_parent)
    {
        lock_guard lock(*parent);
        objects.push_back(parent->shared_from_this());
    }
}

bool Object::EventDispatcher::processEventFilters(Event& event)
{
    auto processFilters = [&event](auto& object)
    {
        lock_guard objectLock(*object);
        auto filter = object->m_filters.find(event.type());
        if (filter == object->m_filters.end())
        {
            return false;
        }

        // Loop thru the filters of the event.
        auto processor = [obj = object.get(), &event](EventTokenPtr token)
        {
            if (!token)
            {
                return false;
            }
            auto filterToken = std::static_pointer_cast<FilterToken>(token);
            event.setHandled(true);
            bool filtered = false;
            {
                ScopeRelock relock(*obj);
                filtered = filterToken->filter()(event);
            }
            if (!filtered)
            {
                event.setHandled(false);
                return false;
            }
            return true;
        };
        auto idx = find_if(filter->second, processor);
        return (idx != std::nullopt);
    };
    auto it = reverse_find_if(objects, processFilters);
    return (it != objects.rend());
}

void Object::EventDispatcher::processEventHandlers(Event& event)
{
    auto processHandlers = [&event](auto& object)
    {
        lock_guard objectLock(*object);
        auto handler = object->m_handlers.find(event.type());
        if (handler == object->m_handlers.end())
        {
            return false;
        }

        auto processor = [obj = object.get(), &event](EventTokenPtr token)
        {
            if (!token)
            {
                return false;
            }
            auto handlerToken = std::static_pointer_cast<HandlerToken>(token);

            // Mark the event consumed.
            event.setHandled(true);
            {
                ScopeRelock relock(*obj);
                CTRACE(event, "process event" << int(event.type()) << "on" << handlerToken->getTarget());
                handlerToken->handler()(event);
            }
            if (event.isHandled())
            {
                return true;
            }
            return false;
        };
        auto idx = find_if(handler->second, processor);
        return (idx != std::nullopt);
    };
    find_if(objects, processHandlers);
}

void Object::dispatchEvent(Event& event)
{
    if (event.isHandled())
    {
        return;
    }

    // Collect the objects
    EventDispatcher dispatcher(*this);
    if (!dispatcher.processEventFilters(event))
    {
        dispatcher.processEventHandlers(event);
    }
}

Object::EventTokenPtr Object::addEventHandler(EventType type, EventHandler handler)
{
    auto token = make_polymorphic_shared<EventToken, HandlerToken>(type, shared_from_this(), std::move(handler));

    lock_guard lock(*this);
    Container::Iterator it = m_handlers.find(type);
    if (it == m_handlers.end())
    {
        TokenList tokens;
        tokens.push_back(token);
        m_handlers.insert(make_pair(type, std::move(tokens)));
    }
    else
    {
        lock_guard ref(it->second);
        it->second.push_back(token);
    }
    return token;
}

Object::EventTokenPtr Object::addEventFilter(EventType type, EventFilter filter)
{
    auto token = make_polymorphic_shared<EventToken, FilterToken>(type, shared_from_this(), std::move(filter));

    lock_guard lock(*this);
    auto it = m_filters.find(type);
    if (it == m_filters.end())
    {
        TokenList tokens;
        tokens.push_back(token);
        m_filters.insert(make_pair(type, std::move(tokens)));
    }
    else
    {
        lock_guard ref(it->second);
        it->second.push_back(token);
    }
    return token;
}



void Object::addChild(Object& child)
{
    ObjectSharedPtr sharedChild = as_shared<Object>(&child);
    OrderedLock lock(this, sharedChild->getParent());

    Object* oldParent = sharedChild->getParent();
    if (oldParent == this)
    {
        return;
    }

    if (oldParent)
    {
        if (oldParent->threadData() != m_threadData)
        {
            throw Exception(ExceptionType::InvalidThreadOwnershipChange);
        }
        ScopeRelock relock(*oldParent);
        oldParent->removeChild(child);
    }

    auto threadMover = [this](Object& object)
    {
        lock_guard lock(object);
        return object.moveToThread(m_threadData);
    };
    child.traverse(threadMover, TraverseOrder::PreOrder);

    m_children.push_back(as_shared<Object>(&child));
    child.m_parent = this;
}

void Object::removeChild(Object& child)
{
    auto childShared = child.shared_from_this();
    OrderedLock lock(this, &child);

    throwIf<ExceptionType::InvalidArgument>(child.getParent() != this);
    throwIf<ExceptionType::NotAChildOfObject>(find(m_children, childShared) == std::nullopt);

    erase(m_children, childShared);
    child.m_parent = nullptr;
}

size_t Object::childCount() const
{
    return m_children.size();
}

Object::VisitResult Object::traverse(const VisitorFunction& visitor, TraverseOrder order)
{
    switch (order)
    {
        case TraverseOrder::PreOrder:
        case TraverseOrder::InversePostOrder:
        {
            VisitResult result = visitor(*this);
            if (result == VisitResult::Continue)
            {
                result = traverseChildren(visitor, order);
            }
            return result;
        }
        case TraverseOrder::PostOrder:
        case TraverseOrder::InversePreOrder:
        {
            VisitResult result = traverseChildren(visitor, order);
            if (result == VisitResult::Abort)
            {
                return result;
            }
            return visitor(*this);
        }
    }

    return VisitResult::Continue;
}

Object::VisitResult Object::traverseChildren(const VisitorFunction& visitor, TraverseOrder order)
{
    VisitResult result = VisitResult::Continue;
    auto visit = [&visitor, &result, order](ObjectSharedPtr child)
    {
        result = child ? child->traverse(visitor, order) : VisitResult::Continue;
        return (result == VisitResult::Abort);
    };

    switch (order)
    {
        case TraverseOrder::PreOrder:
        case TraverseOrder::PostOrder:
        {
            find_if(m_children, visit);
            break;
        }
        case TraverseOrder::InversePreOrder:
        case TraverseOrder::InversePostOrder:
        {
            reverse_find_if(m_children, visit);
            break;
        }
    }

    return result;
}

ThreadDataSharedPtr Object::threadData() const
{
    return m_threadData;
}

LogLine& operator<<(LogLine& log, ObjectSharedPtr ptr)
{
    if (log.isEnabled())
    {
        if (!ptr)
        {
            log << " (null)";
        }
        else
        {
            log << " Object";
        }
    }
    return log;
}

}
