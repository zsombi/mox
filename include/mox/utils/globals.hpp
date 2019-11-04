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

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

// Standard integer types
#include <inttypes.h>

#include <mox/config/platform_config.hpp>

using byte = int8_t;
using long_t = long int;
using ulong_t = unsigned long int;


#ifdef ANDROID

typedef long intptr_t_;

#endif

using Timestamp = std::chrono::system_clock::time_point;

#define FALLTHROUGH     [[fallthrough]]

// unused parameters
#define UNUSED(x)       (void)x

#define FATAL(test, message) \
    if (!(test)) \
    { \
        std::cout << "FATAL: " << __FILE__ << ":" << __LINE__ << " - " << message << std::endl; \
        abort(); \
    }


#ifdef DEBUG
static inline std::string dbg_fileName(const char* path)
{
    std::string fname(path);
    fname = fname.substr(fname.rfind('/') + 1);
    return fname;
}

#   define TRACE(x)     std::cout << dbg_fileName(__FILE__) << " : " << __LINE__ << " :- " << x << std::endl;
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

namespace mox
{

/// Creates a polymorphic shared pointer. Wipes the code bloat caused by the shared pointer.
template <typename BaseType, typename Type, typename... Args>
std::shared_ptr<Type> make_polymorphic_shared(Args&&... args)
{
    static_assert(std::is_base_of_v<BaseType, Type>, "BaseType is not a superclass of Type.");
    auto shared = std::shared_ptr<BaseType>{std::unique_ptr<BaseType>(std::make_unique<Type>(std::forward<Args>(args)...))};
    return std::static_pointer_cast<Type>(shared);
}

template <typename Type, typename Allocator, typename VType>
void erase(std::vector<Type, Allocator>& v, const VType& value)
{
    v.erase(std::remove(v.begin(), v.end(), value), v.end());
}

/// Vector utility, removes the occurences for which the predicate gives affirmative result.
template <typename Type, typename Allocator, typename Predicate>
void erase_if(std::vector<Type, Allocator>& v, const Predicate& predicate)
{
    v.erase(std::remove_if(v.begin(), v.end(), predicate), v.end());
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

} // mox

#endif // GLOBALS
