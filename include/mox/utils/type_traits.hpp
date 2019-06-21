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

#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <type_traits>

namespace mox
{

template <typename T>
struct is_cstring : public std::false_type
{
};

template <typename T>
struct is_cstring<T const*> : public is_cstring<T*>
{
};

template <typename T>
struct is_cstring<T const* const> : public is_cstring<T*>
{
};

template <>
struct is_cstring<char*> : public std::true_type
{
};

template <int N>
struct is_cstring<char[N]> : public std::true_type
{
};


//namespace
//{

//template <typename T, typename U>
//constexpr size_t offsetOf_impl(T const* t, U T::* a)
//{
//    return (char const*)t - (char const*)&(t->*a) >= 0 ?
//               (char const*)t - (char const*)&(t->*a)      :
//               (char const*)&(t->*a) - (char const*)t;
//}

//} // noname

//#define offsetOf(Type, Attr)    offsetOf_impl((Type const*)nullptr, &Type::Attr)

template <typename T, typename U>
constexpr size_t offset_of(U T::*member)
{
    return (char*)&((T*)nullptr->*member) - (char*)nullptr;
}

} // namespace mox

#endif // TYPE_TRAITS_HPP
