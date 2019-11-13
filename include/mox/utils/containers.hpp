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

template <typename T>
struct ZeroCheck
{
    bool operator()(const T& value)
    {
        return !value;
    }
};

template <typename Type, typename CompactingPredicate = ZeroCheck<Type>>
class SharedVector
{
    using Container = std::vector<Type>;

    Container m_container;
    int m_refCount = 0;

    DISABLE_COPY(SharedVector)

public:
    explicit SharedVector() = default;

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
            mox::erase_if(m_container, CompactingPredicate());
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
    const Type& back() const
    {
        return m_container.back();
    }

    Type& operator[](size_t index)
    {
        return m_container[index];
    }
    const Type& operator[](size_t index) const
    {
        return m_container[index];
    }

    void append(Type value)
    {
        m_container.push_back(value);
    }

    void emplace(Type&& value)
    {
        m_container.emplace_back(std::forward<Type>(value));
    }

    template <typename Predicate>
    std::optional<size_t> findIf(Predicate predicate)
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

    template <typename Callback>
    void forEach(Callback callback) const
    {
        auto end = m_container.cend();
        for (auto it = m_container.cbegin(); it != end; ++it)
        {
            callback(*it);
        }
    }
};

}

#endif // CONTAINERS_HPP
