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

#ifndef PROPERTY_TYPE_HPP
#define PROPERTY_TYPE_HPP

#include <mox/config/deftypes.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/utils/flat_map.hpp>
#include <mox/metadata/variant.hpp>
#include <mox/signal/signal_type.hpp>

namespace mox
{

class Property;

/// PropertyAccess defines the access types of a property in Mox.
enum class PropertyAccess
{
    /// Identifies a read-only property.
    ReadOnly,
    /// Identifies a read-write proeprty.
    ReadWrite
};

/// PropertyType declares the type identification of a property. Holds the metatype of the
/// property data, the change signal type and the instances of the property.
class MOX_API PropertyType : public ObjectLock
{
public:
    /// Destructor.
    ~PropertyType() = default;

    /// Returns the change signal type of the property. Both read-only and read-write properties
    /// have change signals.
    /// \return The reference to the signal type.
    SignalType& getChangedSignalType();

    /// Returns the access type of the property.
    /// \return The access type of the property.
    /// \see PropertyAccess
    PropertyAccess getAccess() const;

    /// Adds the instance of the property to track.
    void addPropertyInstance(Property& property);
    /// Removes the instance of the property.
    void removePropertyInstance(Property& property);

    Variant get(intptr_t instance) const;
    bool set(intptr_t instance, const Variant& value);

protected:
    /// Constructor, creates a property type.
    /// \param typeDes The variant type descriptor of the type.
    /// \param signalType The reference to the signal type defining the change signal of the property.
    /// \param access The access type of the property.
    PropertyType(VariantDescriptor&& typeDes, SignalType& signalType, PropertyAccess access);

    /// Map of the property instances.
    FlatMap<intptr_t, Property*> m_instances;
    /// The type descriptor of the property.
    VariantDescriptor m_typeDescriptor;
    /// The reference to the change signal type.
    SignalType& m_changedSignal;
    /// The property access type.
    PropertyAccess m_access;
};

/// Property declarator template. Use this to define the property types.
/// \tparam ValueType The value type of the property.
/// \tparam access The access value of the property.
template <typename ValueType, PropertyAccess access>
class PropertyTypeDecl : public PropertyType
{
    DISABLE_COPY(PropertyTypeDecl)

public:
    /// The change signal type.
    SignalTypeDecl<ValueType> ChangedSignalType;

    /// Constructor.
    PropertyTypeDecl()
        : PropertyType(VariantDescriptor::get<ValueType>(), ChangedSignalType, access)
    {
    }
};

} //namespace mox

#endif // PROPERTY_TYPE_HPP
