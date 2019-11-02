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
#include <mox/metadata/callable.hpp>
#include <mox/module/thread_loop.hpp>

namespace mox
{

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
            TRACE("Destroying thread object's parent, exit thread and join.")
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

void Object::addChild(Object& child)
{
    ObjectSharedPtr sharedChild = child.shared_from_this();
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
            throw thread_differs();
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

    m_children.push_back(child.shared_from_this());
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
    FATAL(index < m_children.size(), "Child index out of range")
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
        TRACE("Object is not a child of the object!");
        throw Callable::invalid_argument();
    }

    OrderedLock lock(this, const_cast<Object*>(&child));
    for (auto it = m_children.cbegin(); it != m_children.cend(); ++it)
    {
        if (it->get() == &child)
        {
            return static_cast<size_t>(std::distance(m_children.cbegin(), it));
        }
    }

    throw Callable::invalid_argument();
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

}
