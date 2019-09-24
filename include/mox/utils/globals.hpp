/*
 * Copyright (C) 2017-2018 bitWelder
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

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <iostream>
#include <memory>

// Standard integer types
#include <inttypes.h>

#include <mox/config/platform_config.hpp>

using byte = int8_t;
using long_t = long int;
using ulong_t = unsigned long int;


#ifdef ANDROID

typedef long intptr_t_;

#endif

// unused parameters
#define UNUSED(x)       (void)x

#define FATAL(test, message) \
    if (!(test)) \
    { \
        std::cout << "FATAL: " << __FILE__ << ":" << __LINE__ << " - " << message << std::endl; \
        abort(); \
    }


#ifdef DEBUG
#   define TRACE(x)     std::cout << x << std::endl;
#else
#   define TRACE(x)
#endif

//
// disable copy construction and operator
//
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete;\
    Class& operator=(const Class&) = delete;
#define DISABLE_MOVE(Class) \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

/// Creates a polymorphic shared pointer.
template <typename BaseType, typename Type, typename... Args>
std::shared_ptr<Type> make_polymorphic_shared(Args&&... args)
{
    static_assert(std::is_base_of_v<BaseType, Type>, "BaseType is not a superclass of Type.");
    Type *p = new Type(std::forward<Args>(args)...);
    std::shared_ptr<BaseType> baseShared(static_cast<BaseType*>(p));
    return std::static_pointer_cast<Type>(baseShared);
}

/// Returns the address of a method.
template <class Class, typename Ret, typename... Args>
void* address(Ret(Class::*func)(Args...))
{
    return (void*&)func;
}

/// Const method version of address().
template <class Class, typename Ret, typename... Args>
void* address(Ret(Class::*func)(Args...) const)
{
    return (void*&)func;
}

template <typename Ret, typename... Args>
void* address(Ret(*func)(Args...))
{
    return (void*&)func;
}

template <typename Functor>
void* address(const Functor& fn)
{
    return (void*&)fn;
}

#endif // GLOBALS
