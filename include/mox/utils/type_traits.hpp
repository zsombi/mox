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
#include <typeinfo>
#include <memory>
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

template <typename T>
struct remove_cvref
{
    typedef std::remove_const_t<std::remove_reference_t<T>> type;
};
template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T>
const std::type_info& getNakedTypeInfo()
{
    typedef typename remove_cvref<T>::type NakedType;
    return typeid(NakedType);
}

/// Returns the N'th type of a variadic template argument>
template <std::size_t N, typename... Args>
using get_type = typename std::tuple_element<N, std::tuple<Args...>>::type;


/// \name Shared pointer tester
/// \{
template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
/// \}

/// \name StaticMetaClass testers
/// \{
///
/// Test whether the T class has static metaclass
/// \tparam T The class type
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
/// \}

/// Tests the existence of whether the T class has a Converter declared.
/// \tparam T The class to test.
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
/// \}

} // namespace mox

#endif // TYPE_TRAITS_HPP
