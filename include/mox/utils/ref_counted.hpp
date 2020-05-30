/*
 * Copyright (C) 2017-2020 bitWelder
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

#ifndef REF_COUNTED_HPP
#define REF_COUNTED_HPP

#include <utility>
#include <mox/utils/type_traits.hpp>
#include <atomic>

namespace mox
{

template <typename T>
void default_retain(remove_cvref_t<T>& v)
{
    ++v;
}

template <typename T>
void default_release(remove_cvref_t<T>& v)
{
    --v;
}

template <typename T>
void default_atomic_retain(std::atomic<remove_cvref_t<T>>& v)
{
    ++v;
}

template <typename T>
void default_atomic_release(std::atomic<remove_cvref_t<T>>& v)
{
    --v;
}

/// Template for reference counted elements, with a retain and release functions.
template <typename T,
          void (*RetainFunction)(remove_cvref_t<T>&) = default_retain<T>,
          void (*ReleaseFunction)(remove_cvref_t<T>&) = default_release<T>>
class RefCounted
{
    using Type = remove_cvref_t<T>;

public:
    /// Construct the reference counted from a value.
    RefCounted(const Type& t = Type())
        : m_value(t)
    {
    }
    /// Move constructor.
    RefCounted(RefCounted&& other)
        : m_value(other.m_value)
    {
        other.value = Type();
    }
    /// Copy constructor.
    RefCounted(const RefCounted& other)
        : m_value(other.m_value)
    {
        RetainFunction(m_value);
    }
    /// Destructor.
    ~RefCounted()
    {
        ReleaseFunction(m_value);
    }
    /// Swaps two reference counted values.
    void swap(RefCounted& other) noexcept
    {
        std::swap(m_value, other.m_value);
    }
    /// Copy assignment operator.
    RefCounted& operator=(const RefCounted& other)
    {
        RefCounted copy(other);
        swap(copy);
        return *this;
    }
    /// Move assignment operator.
    RefCounted& operator=(RefCounted&& other)
    {
        RefCounted moved(std::move(other));
        swap(moved);
        return *this;
    }
    /// Cast operator.
    operator T() const
    {
        return m_value;
    }
    /// Self retainer function.
    void retain()
    {
        RetainFunction(m_value);
    }
    /// Self release function.
    void release()
    {
        ReleaseFunction(m_value);
    }

protected:
    /// The reference counted value.
    T m_value;
};

/// Template for reference counted atomic elements, with a retain and release functions.
template <typename T,
          void (*RetainFunction)(std::atomic<remove_cvref_t<T>>&) = default_atomic_retain<T>,
          void (*ReleaseFunction)(std::atomic<remove_cvref_t<T>>&) = default_atomic_release<T>>
class AtomicRefCounted
{
    using RefType = remove_cvref_t<T>;

public:
    /// Construct the reference counted from a value.
    AtomicRefCounted(const RefType& t = RefType())
        : m_value(t)
    {
    }
    /// Move constructor.
    AtomicRefCounted(AtomicRefCounted&& other)
        : m_value(other.m_value)
    {
        other.value = RefType();
    }
    /// Copy constructor.
    AtomicRefCounted(const AtomicRefCounted& other)
        : m_value(other.m_value)
    {
        RetainFunction(m_value);
    }
    /// Destructor.
    ~AtomicRefCounted()
    {
        ReleaseFunction(m_value);
    }
    /// Swaps two reference counted values.
    void swap(AtomicRefCounted& other) noexcept
    {
        std::swap(m_value, other.m_value);
    }
    /// Copy assignment operator.
    AtomicRefCounted& operator=(const AtomicRefCounted& other)
    {
        RefCounted copy(other);
        swap(copy);
        return *this;
    }
    /// Move assignment operator.
    AtomicRefCounted& operator=(AtomicRefCounted&& other)
    {
        AtomicRefCounted moved(std::move(other));
        swap(moved);
        return *this;
    }
    /// Cast operator.
    operator T() const
    {
        return m_value;
    }
    /// Self retainer function.
    void retain()
    {
        RetainFunction(m_value);
    }
    /// Self release function.
    void release()
    {
        ReleaseFunction(m_value);
    }

protected:
    /// The reference counted value.
    std::atomic<RefType> m_value;
};

/// Template for reference counted types.
/// \tparam RefCountedType The reference counted type, must derive from RefCountedType template
/// or at least have retain() and release() methods.
template <class RefCountedType>
class RefCounter
{
public:
    /// Constructs a reference counter from \a self, and retains \a self.
    RefCounter(RefCountedType& self)
        : m_refCounted(self)
    {
        m_refCounted.retain();
    }
    /// Destructs the reference counter releasing the reference counted element.
    ~RefCounter()
    {
        m_refCounted.release();
    }

protected:
    /// The reference counted element.
    RefCountedType& m_refCounted;
};

} // mox

#endif // REF_COUNTED_HPP
