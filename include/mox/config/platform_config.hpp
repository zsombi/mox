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

#ifndef PLATFORM_CONFIG_HPP
#define PLATFORM_CONFIG_HPP

// Compiler checks.
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
#   define LONG_SYNONIM_OF_UINT64
#   else
#       define CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#   endif
#   define EXCEPTION_NOEXCEPT          _NOEXCEPT
#   define DECLARE_NOEXCEPT            _NOEXCEPT
#   define DECLARE_NOEXCEPTX(x)
#elif defined(__GNUC__) || defined(__GLIBCXX__)
#   define EXCEPTION_NOEXCEPT          _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_USE_NOEXCEPT
#   define DECLARE_NOEXCEPT            _GLIBCXX_USE_NOEXCEPT
#   define DECLARE_NOEXCEPTX(x)        noexcept(x)
#endif // GNUC


// OSX and Linux use the same declaration mode for inport and export
#define DECL_EXPORT         __attribute__((visibility("default")))
#define DECL_IMPORT         __attribute__((visibility("default")))

#ifdef MOX_LIBRARY
#   define MOX_API     DECL_EXPORT
#else
#   define MOX_API     DECL_IMPORT
#endif

#define FALLTHROUGH     [[fallthrough]]

// unused parameters
#define UNUSED(x)       (void)x

//
// disable copy construction and operator
//
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete;\
    Class& operator=(const Class&) = delete;
#define DISABLE_MOVE(Class) \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

#define DISABLE_COPY_OR_MOVE(Class) \
    DISABLE_COPY(Class) \
    DISABLE_MOVE(Class)

#endif // PLATFORM_CONFIG_HPP
