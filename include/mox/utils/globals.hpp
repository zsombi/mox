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

#include <mutex>
#include <iostream>
#include <memory>
#include <bitset>
#include <cstddef>  // std::byte

// OSX and Linux use the same declaration mode for inport and export
#define DECL_EXPORT         __attribute__((visibility("default")))
#define DECL_IMPORT         __attribute__((visibility("default")))

#ifdef MOX_LIBRARY
#   define MOX_API     DECL_EXPORT
#else
#   define MOX_API     DECL_IMPORT
#endif

// Compiler checks
#ifdef _MSVC_LANG
// TODO: dig MSVC version that supports c++17
#elif (__cplusplus < 201703L)

#error "c++17 is required"

#endif

#if defined(__clang__)
#   if defined(__apple_build_version__)
#       /* http://en.wikipedia.org/wiki/Xcode#Toolchain_Versions */
#       if __apple_build_version__ >= 7000053
#           define CC_CLANG 306
#       elif __apple_build_version__ >= 6000051
#           define CC_CLANG 305
#       elif __apple_build_version__ >= 5030038
#           define CC_CLANG 304
#       elif __apple_build_version__ >= 5000275
#           define CC_CLANG 303
#       elif __apple_build_version__ >= 4250024
#           define CC_CLANG 302
#       elif __apple_build_version__ >= 3180045
#           define CC_CLANG 301
#       elif __apple_build_version__ >= 2111001
#           define CC_CLANG 300
#       else
#           error "Unknown Apple Clang version"
#       endif
#   else
#       define CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#   endif
#   define DECLARE_NOEXCEPT
#   define DECLARE_NOEXCEPTX(x)
#else
#   define DECLARE_NOEXCEPT            noexcept
#   define DECLARE_NOEXCEPTX(x)        noexcept(x)
#endif

#ifdef _U_LONG

typedef u_long ulong;

#endif

#ifdef ANDROID

typedef unsigned long ulong;
typedef long intptr_t_;

#else

typedef intptr_t intptr_t_;
using std::byte;

#endif

// unused parameters
#define UNUSED(x)       (void)x

// assert
#define ASSERT(test, message) \
    if (!(test)) \
    { \
        std::cout << "ASSERT: " << __FILE__ << ":" << __LINE__ << " - " << message << std::endl; \
        abort(); \
    }

//
// disable copy construction and operator
//
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete;\
    Class& operator=(const Class&) = delete;
#define DISABLE_MOVE(Class) \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;
//
// Private class implementation support
//

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr.get()); } \
    friend class Class##Private;

#define DECLARE_PRIVATE_D(Dptr, Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(Dptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(Dptr); } \
    friend class Class##Private;

#define DECLARE_PUBLIC(Class)                                    \
    public: \
    static Class##Private* get(Class& object) { return object.d_func(); } \
    static const Class##Private* cget(const Class& object) { return object.d_func(); } \
    static Class##Private* get(const Class& object) { return const_cast<Class*>(&object)->d_func(); } \
    static Class##Private* get(std::shared_ptr<Class> object) { return object->d_func(); } \
    static const Class##Private* get(std::shared_ptr<const Class> object) { return object->d_func(); } \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define D_PTR(Class) \
    Class##Private * const d = d_func()

#define Q_PTR(Class) \
    Class * const q = q_func()

namespace mox
{

/// Flags. An extension to std::bitset to test multiple bits
template <size_t bitcount>
class Flags : public std::bitset<bitcount>
{
public:
    bool test_all(std::initializer_list<size_t> bits)
    {
        bool result = false;
        for (auto bit : bits)
        {
            result |= this->test(bit);
        }
        return result;
    }
};

} // namespace mox

/// Creates a polymorphic shared pointer.
template <typename BaseType, typename Type, typename... Args>
std::shared_ptr<Type> make_polymorphic_shared(Args... args)
{
    Type *p = new Type(args...);
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

typedef std::unique_lock<std::mutex> MutexLock;

#endif // GLOBALS
