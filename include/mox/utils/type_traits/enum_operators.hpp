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

#ifndef ENUM_OPERATORS_HPP
#define ENUM_OPERATORS_HPP

#include <type_traits>

namespace mox
{

/// Enum class operators
template <typename T>
struct enable_enum_operators : public std::false_type
{
};

/// Increment operator for enum class.
template <typename Enum>
std::enable_if_t<enable_enum_operators<Enum>::value, Enum>& operator ++(Enum& enumerator)
{
    using underlying_type = std::underlying_type_t<Enum>;
    enumerator = static_cast<Enum>(static_cast<underlying_type>(enumerator) + 1);
    return enumerator;
}

/// Decrement operator for enum class.
template <typename Enum>
std::enable_if_t<enable_enum_operators<Enum>::value, Enum>& operator --(Enum& enumerator)
{
    using underlying_type = std::underlying_type_t<Enum>;
    enumerator = static_cast<Enum>(static_cast<underlying_type>(enumerator) - 1);
    return enumerator;
}

/// Use this macro to enable enum class operators.
#define ENABLE_ENUM_OPERATORS(EnumType) \
template <> \
struct enable_enum_operators<EnumType> : public std::true_type \
{ \
};

}

#endif // ENUM_OPERATORS_HPP
