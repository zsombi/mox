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

#ifndef METATYPE_HPP
#define METATYPE_HPP

#include <any>
#include <mox/utils/type_traits/enum_operators.hpp>

namespace mox
{

/// Defines the type identifier. User defined types are registered in the
/// user area, right after UserType.
enum class Metatype : int
{
    Invalid = -1,
    // void is a weirdo type
    Void = 0,
    Bool,
    Char,
    Byte,
    Short,
    Word,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    NumericMax = Double, // Identifies the maximum of the numeric types.
    //
    String,
    Literal,
    // Pointer types
    VoidPtr,
    BytePtr,
    Int32Ptr,
    Int64Ptr,
    // Vectors
    Int32Vector,
    // All user types to be installed here
    UserType
};
ENABLE_ENUM_OPERATORS(Metatype)

/// Base type of all Mox metavalues.
typedef std::any MetaValue;

} // mox

#endif // METATYPE_HPP
