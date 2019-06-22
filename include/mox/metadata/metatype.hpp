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

#include <typeindex>
#include <typeinfo>
#include <type_traits>

namespace mox
{

struct MetatypeDescriptor;

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
    Int,
    UInt,
    Long,
    ULong,
    Int64,
    UInt64,
    Float,
    Double,
    //
    String,
    MetaObject,
    VoidPtr,
    CString,
    // All user types to be installed here
    UserType
};

namespace registrar
{
/// Finds a MetatypeDescriptor associated to the \a rtti.
/// \return nullptr if the \e rtti does not have any associated MetatypeDescriptor registered.
const MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti);

/// Finds a Metatype associated to the \a rtti.
/// \return The metatype identifier of the RTTI.
Metatype findMetatype(const std::type_info& rtti);

/// Registers a MetatypeDescriptor associated to the \a rtti.
/// \param rtti The type info of the type to register.
/// \param isEnum True if the type defines an enum.
/// \return the MetatypeDescriptor associated to the \e rtti.
Metatype tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass);

}

/// Returns the metatype identifier of the given type. The function asserts if
/// the type is not registered in the metatype system.
/// Example:
/// \code
/// Metatype type = metaType<int*>();
/// \endcode
template <typename Type>
Metatype metaType();

template <typename Type>
const MetatypeDescriptor& metatypeDescriptor();

/// Registers a Type into the Mox metatype subsystem. The function returns the
/// Metatype if already registered.
/// \return The Metatype handler of the Type.
template <typename Type>
Metatype registerMetaType();

} // mox

#include <mox/metadata/detail/metatype_impl.hpp>


#endif // METATYPE_HPP
