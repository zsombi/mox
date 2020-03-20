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

#include <mox/object.hpp>
#include <mox/meta/core/callable.hpp>
#include <mox/module/thread_loop.hpp>

#if MOX_ENABLE_LOGGING == ON
#include <mox/meta/core/metatype_descriptor.hpp>
#include <mox/meta/class/metaclass.hpp>
#endif

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

/******************************************************************************
 * Object
 */
Object::Object()
    : m_threadData(ThreadData::thisThreadData())
{
}

void Object::removeChildren()
{
    lock_guard lock(*this);

    while (!m_children.empty())
    {
        // If the child is a thread, the thread must be stopped before it gets removed from this object.
        ThreadLoopSharedPtr thread = std::dynamic_pointer_cast<ThreadLoop>(m_children.back());

        // Remove child from parent first.
        {
            ScopeRelock relock(*this);
            removeChildAt(m_children.size() - 1);
        }

        if (thread)
        {
            CTRACE(object, "Destroying thread object's parent, exit thread and join.");
            thread->exitAndJoin();
        }
    }
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

Object* Object::parent() const
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
    for (auto parent = &target; parent; parent = parent->m_parent)
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
    reverse_find_if(objects, processHandlers);
}

void Object::dispatchEvent(Event& event)
{
    if (event.isHandled())
    {
        return;
    }

    // Check default handlers.
    if (event.type() == EventType::DeferredSignal)
    {
        DeferredSignalEvent& deferredSignal = dynamic_cast<DeferredSignalEvent&>(event);
        deferredSignal.activate();
        event.setHandled(true);
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
    OrderedLock lock(this, sharedChild->parent());

    Object* oldParent = sharedChild->parent();
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
    size_t index = childIndex(child);
    OrderedLock lock(this, &child);
    m_children.erase(m_children.begin() + int(index));
    child.m_parent = nullptr;
}

void Object::removeChildAt(size_t index)
{
    FATAL(index < m_children.size(), "Child index out of range");
    {
        Object* child = m_children[index].get();
        OrderedLock lock(this, child);
        child->m_parent = nullptr;
    }
    m_children.erase(m_children.begin() + int(index));
}

size_t Object::childCount() const
{
    return m_children.size();
}

size_t Object::childIndex(const Object& child)
{
    if (child.parent() != this)
    {
        CTRACE(object, "Object is not a child of the object!");
        throw Exception(ExceptionType::InvalidArgument);
    }

    OrderedLock lock(this, const_cast<Object*>(&child));
    for (auto it = m_children.cbegin(); it != m_children.cend(); ++it)
    {
        if (it->get() == &child)
        {
            return static_cast<size_t>(std::distance(m_children.cbegin(), it));
        }
    }

    throw Exception(ExceptionType::InvalidArgument);
}

ObjectSharedPtr Object::childAt(size_t index)
{
    lock_guard lock(*this);
    if (index > m_children.size())
    {
        return nullptr;
    }
    return m_children[index];
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
        result = child->traverse(visitor, order);
        return (result == VisitResult::Abort);
    };

    switch (order)
    {
        case TraverseOrder::PreOrder:
        case TraverseOrder::PostOrder:
        {
            std::find_if(m_children.begin(), m_children.end(), visit);
            break;
        }
        case TraverseOrder::InversePreOrder:
        case TraverseOrder::InversePostOrder:
        {
            std::find_if(m_children.rbegin(), m_children.rend(), visit);
            break;
        }
    }

    return result;
}

ThreadDataSharedPtr Object::threadData() const
{
    return m_threadData;
}

#if defined(MOX_ENABLE_LOGS)
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
            const auto* mc = ptr->__getStaticMetaClass();
            auto type = mc->getMetaTypes().first;
            const auto& typeInfo = MetatypeDescriptor::get(type);
            log << ' ' << typeInfo.name();
        }
    }
    return log;
}
#endif
}
