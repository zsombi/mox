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

namespace mox
{

Object::Object()
{
}

Object::~Object()
{
    if (m_parent)
    {
        m_parent->removeChild(*this);
    }
}

ObjectSharedPtr Object::create(Object* parent)
{
    return createObject(new Object, parent);
}

Object* Object::parent() const
{
    return m_parent;
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
        ScopeUnlock relock(*oldParent);
        oldParent->removeChild(child);
    }

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

}
