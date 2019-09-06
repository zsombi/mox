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
#include <mox/utils/type_traits/enum_operators.hpp>

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


template <typename T, typename U>
constexpr size_t offset_of(U T::*member)
{
    return (char*)&((T*)nullptr->*member) - (char*)nullptr;
}

/// Tests whether the T class has a StaticMetaClass declared.
template <class T>
struct has_static_metaclass
{
    typedef char yes;
    typedef long no;

    template <class C>
    static yes test(typename C::StaticMetaClass* = 0);
    template <class C>
    static no test(...);

    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
};

template <typename T>
inline constexpr bool has_static_metaclass_v = has_static_metaclass<T>::value;

/// Tests whether the T class has dynamic metaclass getter.
template <typename T>
struct has_dynamic_metaclass
{
private:
    typedef char yes_type;
    typedef long no_type;
    template <typename U> static yes_type test(decltype(&U::getMetaClass));
    template <typename U> static no_type  test(...);
public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);
};

template <typename T>
inline constexpr bool has_dynamic_metaclass_v = has_dynamic_metaclass<T>::value;

/// Tests whether the T class has a Converter declared.
template <class T>
struct has_converter
{
    typedef char yes;
    typedef long no;

    template <class C>
    static yes test(typename C::Converter* = 0);
    template <class C>
    static no test(...);

    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
};

template <typename T>
inline constexpr bool has_converter_v = has_converter<T>::value;

} // namespace mox

#endif // TYPE_TRAITS_HPP
