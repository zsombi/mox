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

#ifndef METATYPE_DESCRIPTOR_HPP
#define METATYPE_DESCRIPTOR_HPP

#include <array>
#include <vector>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <mox/utils/flat_map.hpp>
#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox {

/// The MetatypeDescriptor class extends the RTTI of the types in Mox. Provides information
/// about the type, such as constness, whether is a pointer or enum. It also stores
/// a fully qualified name of the type. The MetatypeDescriptor is also used when comparing
/// arguments passed on invocation with the arguments of the metamethods.
///
/// The class is a standalone class and cannot be derived.
struct MOX_API MetatypeDescriptor
{
public:
    /// Destructor.
    virtual ~MetatypeDescriptor() final;

    /// Checks whether the Type is a custom metatype.
    bool isCustomType();

    /// Returns the MetatypeDescriptor of a given type identifier.
    /// \param typeId The type identifier.
    /// \return The registered MetatypeDescriptor holding the typeId.
    static const MetatypeDescriptor& get(Metatype typeId);

    /// Checks whether this metatype is the supertype of the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is the supertype of the type, \e false if not.
    bool isSupertypeOf(const MetatypeDescriptor& type) const;

    /// Checks whether this metatype is derived from the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is derived from the type, \e false if not.
    bool derivesFrom(const MetatypeDescriptor& type) const;

    /// Returns \e true if the MetatypeDescriptor holds a valid type.
    bool isValid() const;

    /// Returns \e true if the MetatypeDescriptor holds the void value.
    /// \note void pointers are reported as a separate type.
    bool isVoid() const;

    /// Returns the type identifier held by the MetatypeDescriptor.
    Metatype id() const;

    /// Returns the fully qualified name of the MetatypeDescriptor.
    const char* name() const;

    /// Returns \e true if the type held by the MetatypeDescriptor is an enumeration.
    bool isEnum() const;

    /// Returns \e true if the type held by this MetatypeDescriptor is a class.
    bool isClass() const;

    /// Return \e true if the type held by this MetatypeDescriptor is a pointer.
    bool isPointer() const;

    /// Returns the RTTI (Run-time Type Information) of the MetatypeDescriptor.
    const std::type_info* rtti() const;

    MetatypeConverter* findConverterTo(Metatype target);

    bool addConverter(MetatypeConverterPtr&& converter, Metatype target);

private:
    /// MetatypeDescriptor constructor.
    explicit MetatypeDescriptor(std::string_view name, int id, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer);

    typedef FlatMap<Metatype, MetatypeConverterPtr> ConverterMap;

    ConverterMap m_converters;
    char* m_name{nullptr};
    const std::type_info* m_rtti{nullptr};
    Metatype m_id{Metatype::Invalid};
    bool m_isEnum:1;
    bool m_isClass:1;
    bool m_isPointer:1;

    friend struct MetaData;
    friend const MetatypeDescriptor* tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass);

    DISABLE_COPY(MetatypeDescriptor)
    DISABLE_MOVE(MetatypeDescriptor)
};

} // namespace mox

#endif // METATYPE_DESCRIPTOR_HPP
