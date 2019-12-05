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

#ifndef METADATA_HPP
#define METADATA_HPP

#include <mox/metadata/metatype.hpp>
#include <mox/utils/globals.hpp>

#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <string_view>

namespace mox {

struct MetatypeDescriptor;
struct MetaClass;

/// Returns the metatype identifier of the given type. The function asserts if
/// the type is not registered in the metatype system.
/// Example:
/// \code
/// Metatype type = metaType<int*>();
/// \endcode
/// \throws mox::Exception with ExceptionType::MetatypeNotRegistered if the type is not registered
/// as metatype
template <typename Type>
Metatype metaType();

/// Returns the descriptor associated to the Type in the metadata.
/// \throws type_not_registered if the Type is not a metatype.
template <typename Type>
const MetatypeDescriptor& metatypeDescriptor();

/// Registers a Type into the Mox metatype subsystem. The function returns the
/// Metatype identifier registered.
/// \tparam Type The type to register as metatype.
/// \param name The optional name to override the deducted type name from RTTI.
/// \return The Metatype handler of the Type.
template <typename Type>
Metatype registerMetaType(std::string_view name = "");

/// Registers a Class with both static and pointer type into the Mox metatype subsystem.
/// The function returns the pair of Metatype identifiers registered, where the first is
/// the static metatype and the second is the pointer metatype.
/// \tparam Class The type to register as static and pointer metatype.
/// \param name The optional name to override the deducted type name from RTTI.
/// \return The pair of Metatype handlers of the Class and Class* types.
template <class Class>
std::pair<Metatype, Metatype> registerClassMetaTypes(std::string_view name = "");

/// Registers a converter function that converts a value between two distinct types.
/// Returns \e true if the converter is registered with success, \e false otherwise.
/// A converter registration fails if Mox already has a converter for the desired types.
template <typename From, typename To, typename Function>
bool registerConverter(Function function);

/// Registers a converter method that converts the instance of the class that holds
/// the method to a given type.
/// Returns \e true if the converter is registered with success, \e false otherwise.
/// A converter registration fails if Mox already has a converter for the desired types.
template <typename From, typename To>
bool registerConverter(To (From::*function)() const);

namespace metadata
{

/// Scans metatypes and returns the metatype for which the \a predicate returns \e true.
/// \param predicate The predicate to test a metatype descriptor.
/// \return The pointer to the MetatypeDescruptor found, nullptr if the predicate didn't find a match.
MOX_API const MetatypeDescriptor* scanMetatypes(std::function<bool(const MetatypeDescriptor&)> predicate);

/// Scans metaclasses and returns the metaclass for which the \a predicate returns \e true.
/// \param predicate The predicate to test a metaclass descriptor.
/// \return The pointer to the MetaClass found, nullptr if the predicate didn't find a match.
MOX_API const MetaClass* scanMetaClasses(std::function<bool(const MetaClass&)> predicate);

/// Finds a MetatypeDescriptor associated to the \a rtti.
/// \return nullptr if the \e rtti does not have any associated MetatypeDescriptor registered.
MOX_API MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti);

/// Finds a Metatype associated to the \a rtti.
/// \return The metatype identifier of the RTTI.
MOX_API Metatype findMetatype(const std::type_info& rtti);

/// Registers a MetatypeDescriptor associated to the \a rtti.
/// \param rtti The type info of the type to register.
/// \param isEnum True if the type defines an enum.
/// \param isClass True if the type is a class.
/// \param isPointer True if the type is a pointer.
/// \param name Optional, the name of the metatype to override the default RTTI type name.
/// \return the MetatypeDescriptor associated to the \e rtti.
MOX_API Metatype tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass, bool isPointer, std::string_view name);

/// Registers a \a converter that converts a value from \a fromType to \a toType.
MOX_API bool registerConverter(MetatypeConverterPtr&& converter, Metatype fromType, Metatype toType);

/// Look for the converter that converts a type between \a from and \a to.
/// \param from The source type.
/// \param to The destination type.
/// \return The converter found that converts a value between \a from and to \a to types.
/// nullptr is returned if there is no converter found to convert between the two metatypes.
MOX_API MetatypeConverter* findConverter(Metatype from, Metatype to);

}} // namespace mox::metadata

#include <mox/metadata/detail/metadata_impl.hpp>

#endif // METADATA_HPP
