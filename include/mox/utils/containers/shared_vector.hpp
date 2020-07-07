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

#include <mox/utils/algorithm.hpp>
#include <mox/utils/type_traits.hpp>
#include <mox/utils/locks.hpp>

namespace mox
{

namespace
{

template <typename T>
struct ZeroCheck
{
    bool operator()(const T& value)
    {
        return !value;
    }
};

template <typename T>
struct ZeroSet
{
    void operator()(T& v)
    {
        if constexpr (mox::is_shared_ptr<T>::value)
        {
            v.reset();
        }
        if constexpr (std::is_pointer_v<T>)
        {
            v = nullptr;
        }
        if constexpr (std::is_integral_v<T>)
        {
            v = 0;
        }
        if constexpr (std::is_class_v<T>)
        {
            v = T();
        }
    }
};

}

template <typename Type,
          typename ValidityChecker = ZeroCheck<Type>,
          typename Invalidator = ZeroSet<Type>>
class MOX_API SharedVector
{
    using Self = SharedVector<Type, ValidityChecker, Invalidator>;

public:
    using ContainerType = std::vector<Type>;

    explicit SharedVector() = default;

    SharedVector(Self&& other)
    {
        *this = std::forward<Self>(other);
    }

    Self& operator=(Self&& other)
    {
        std::swap(m_container, other.m_container);
        std::swap(m_zeroCheck, other.m_zeroCheck);
        std::swap(m_invalidate, other.m_invalidate);
        std::swap(m_refCount, other.m_refCount);
        std::swap(m_dirtyCount, other.m_dirtyCount);
        return *this;
    }

    operator ContainerType() const
    {
        return m_container;
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
        if (--m_refCount <= 0 && m_dirtyCount)
        {
            m_dirtyCount = 0;
            mox::erase_if(m_container, m_zeroCheck);
        }
    }

    bool empty() const
    {
        return !size();
    }

    size_t size() const
    {
        return m_container.size() - m_dirtyCount;
    }

    Type& back()
    {
        return m_container.back();
    }

    void push_back(Type value)
    {
        lock_guard guard(*this);
        m_container.push_back(value);
    }

    template <typename Predicate>
    bool push_back_if(Type value, Predicate predicate)
    {
        lock_guard guard(*this);
        auto idx = std::find_if(m_container.begin(), m_container.end(), predicate);
        if (idx != m_container.end())
        {
            return false;
        }
        push_back(std::forward<Type>(value));
        return true;
    }

    void emplace_back(Type&& value)
    {
        lock_guard guard(*this);
        m_container.emplace_back(std::forward<Type>(value));
    }

    template <typename Predicate>
    bool emplace_back_if(Type&& value, Predicate predicate)
    {
        lock_guard guard(*this);
        auto idx = std::find_if(m_container.begin(), m_container.end(), predicate);
        if (idx != m_container.end())
        {
            return false;
        }
        emplace_back(std::forward<Type>(value));
        return true;
    }

private:
    ContainerType m_container;
    ValidityChecker m_zeroCheck;
    Invalidator m_invalidate;
    int m_refCount = 0;
    int m_dirtyCount = 0;

    DISABLE_COPY(SharedVector)

    template <typename T, typename C, typename I, typename F>
    friend void for_each(SharedVector<T, C, I>&, F);

    template <typename T, typename C, typename I, typename VT>
    friend std::optional<T> find(SharedVector<T, C, I>&, VT);

    template <typename T, typename C, typename I, typename F>
    friend std::optional<T> find_if(SharedVector<T, C, I>&, F);

    template <typename T, typename C, typename I, typename F>
    friend std::optional<T> reverse_find_if(SharedVector<T, C, I>&, F);

    template <typename T, typename C, typename I, typename VT>
    friend void erase(SharedVector<T, C, I>&, const VT&);

    template <typename T, typename C, typename I, typename F>
    friend bool erase_if(SharedVector<T, C, I>&, F);
};


/// Non-member functions of SharedVector

template <typename Type, typename Compact, typename Invalidate, typename Function>
void for_each(SharedVector<Type, Compact, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.end();
    std::for_each(shv.m_container.begin(), end, function);
}

template <typename Type, typename Compact, typename Invalidate, typename VType>
std::optional<Type> find(SharedVector<Type, Compact, Invalidate>& shv, VType value)
{
    lock_guard lock(shv);
    auto end = shv.m_container.end();
    auto it = std::find(shv.m_container.begin(), end, value);
    if (it != shv.m_container.end())
    {
        return std::make_optional(*it);
    }
    return std::nullopt;
}

template <typename Type, typename Compact, typename Invalidate, typename Function>
std::optional<Type> find_if(SharedVector<Type, Compact, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.end();
    auto it = std::find_if(shv.m_container.begin(), end, function);
    if (it != shv.m_container.end())
    {
        return std::make_optional(*it);
    }
    return std::nullopt;
}

template <typename Type, typename Compact, typename Invalidate, typename Function>
std::optional<Type> reverse_find_if(SharedVector<Type, Compact, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.rend();
    auto it = std::find_if(shv.m_container.rbegin(), end, function);
    if (it != shv.m_container.rend())
    {
        return std::make_optional(*it);
    }
    return std::nullopt;
}

template <typename Type, typename Compact, typename Invalidate, typename VType>
void erase(SharedVector<Type, Compact, Invalidate>& shv, const VType& value)
{
    lock_guard lock(shv);
    auto end = shv.m_container.end();
    auto it = std::find(shv.m_container.begin(), end, value);
    if (it != shv.m_container.end())
    {
        shv.m_invalidate(*it);
        shv.m_dirtyCount++;
    }
}

template <typename Type, typename Compact, typename Invalidate, typename Function>
bool erase_if(SharedVector<Type, Compact, Invalidate>& shv, Function predicate)
{
    lock_guard lock(shv);
    auto end = shv.m_container.end();
    auto it = std::find_if(shv.m_container.begin(), end, predicate);
    if (it != shv.m_container.end())
    {
        shv.m_invalidate(*it);
        shv.m_dirtyCount++;
        return true;
    }
    return false;
}

}

#endif // CONTAINERS_HPP
