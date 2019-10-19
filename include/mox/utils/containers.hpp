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

#ifndef CONTAINERS_HPP
#define CONTAINERS_HPP

#include <vector>
#include <memory>
#include <optional>

#include <mox/utils/globals.hpp>

namespace mox
{

template <typename Type>
class LockableContainer
{
    std::vector<Type> m_container;
    int m_refCount = 0;
    bool(*m_compactingPredicate)(const Type&);

    DISABLE_COPY(LockableContainer)

public:
    explicit LockableContainer(bool(*predicate)(const Type&))
        : m_compactingPredicate(predicate)
    {
    }
    ~LockableContainer()
    {
    }

    int lockCount() const
    {
        return m_refCount;
    }

    void lock()
    {
        ++m_refCount;
    }

    void unlock()
    {
        if (--m_refCount <= 0)
        {
            mox::erase_if(m_container, m_compactingPredicate);
        }
    }

    void clear()
    {
        m_container.clear();
    }

    size_t size() const
    {
        return m_container.size();
    }

    Type& back()
    {
        return m_container.back();
    }

    Type& operator[](size_t index)
    {
        return m_container[index];
    }

    void push_back(Type value)
    {
        m_container.push_back(value);
    }

    void emplace_back(Type&& value)
    {
        m_container.emplace_back(std::forward<Type>(value));
    }

    template <typename Predicate>
    std::optional<size_t> find(Predicate predicate)
    {
        auto it = std::find_if(m_container.begin(), m_container.end(), predicate);
        if (it == m_container.end())
        {
            return std::nullopt;
        }
        return std::distance(m_container.begin(), it);
    }

    template <typename Callback>
    void forEach(Callback callback)
    {
        auto end = m_container.end();
        for (auto it = m_container.begin(); it != end; ++it)
        {
            callback(*it);
        }
    }
};

}

#endif // CONTAINERS_HPP
