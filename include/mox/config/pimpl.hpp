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

#include <memory>

#ifndef PIMPL_HPP
#define PIMPL_HPP


template <typename T>
T* pimplGetPtrHelper(T* ptr)
{
    return ptr;
}

template <typename Ptr>
auto pimplGetPtrHelper(const Ptr& ptr) -> decltype(ptr.operator->())
{
    return ptr.operator->();
}

template <typename Ptr>
auto pimplGetPtrHelper(Ptr& ptr) -> decltype(ptr.operator->())
{
    return ptr.operator->();
}

#define DECLARE_GETTERS(Class) \
    static Class##Private* get(Class& p) \
    { \
        return p.d_func(); \
    } \
    static const Class##Private* get(const Class& p) \
    { \
        return p.d_func(); \
    }

#define DECLARE_PRIVATE(Class)                                                      \
    inline Class##Private* d_func()                                                 \
    {                                                                               \
        return reinterpret_cast<Class##Private*>(pimplGetPtrHelper(d_ptr));         \
    }                                                                               \
    inline const Class##Private* d_func() const                                     \
    {                                                                               \
        return reinterpret_cast<const Class##Private*>(pimplGetPtrHelper(d_ptr));   \
    }                                                                               \
    friend class Class##Private;

#define DECLARE_PUBLIC(Class)                                                       \
    inline Class* p_func()                                                          \
    {                                                                               \
        return static_cast<Class*>(p_ptr);                                          \
    }                                                                               \
    inline const Class* p_func() const                                              \
    {                                                                               \
        return static_cast<const Class *>(p_ptr);                                   \
    }                                                                               \
    friend class Class;                                                             \
    DECLARE_GETTERS(Class)


#define D_PTR(Class) \
    auto const d = d_func()
#define D() \
    auto const d = d_func()

#define P_PTR(Class) \
    auto const p = p_func()
#define P() \
    auto const p = p_func()


/// Movable PIMPL.
namespace pimpl
{

/// The D-pointer type.
template <class T>
using d_ptr_type = std::unique_ptr<T>;

template <class T, typename... Args>
inline d_ptr_type<T> make_d_ptr(Args&&... args)
{
    static_assert(!std::is_array<T>::value, "d_ptr_type does not support arrays");
    return d_ptr_type<T>(new T(std::forward<Args>(args)...));
}

} // pimpl

#define DECLARE_PRIVATE_PTR(Class)          \
    pimpl::d_ptr_type<Class##Private> d_ptr;\
    DECLARE_PRIVATE(Class)                  \

#define DECLARE_PUBLIC_PTR(Class)   \
    Class* p_ptr = nullptr;         \
    DECLARE_PUBLIC(Class)

#endif // PIMPL_HPP
