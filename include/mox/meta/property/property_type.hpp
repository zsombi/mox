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

#include <mox/config/platform_config.hpp>
#include <mox/metatype.core/variant.hpp>
#include <mox/meta/metabase/metabase.hpp>
#include <mox/meta/signal/signal_type.hpp>
#include <mox/meta/property/property_data.hpp>

namespace mox
{

class Property;

/// PropertyAccess defines the access types of a property in Mox.
enum class PropertyAccess
{
    /// Identifies a read-only property.
    ReadOnly,
    /// Identifies a read-write property.
    ReadWrite
};

/// PropertyType declares the type identification of a property, and holds the metatype of the
/// property data, the change signal type associated to the property, and the default value of
/// the property type. You must declare the change signal type separately and before declaring
/// the property type.
///
/// You must declare a property type before using properties in Mox. You can declare property
/// types using PropertyTypeDecl<> template class.
///
/// To declare a property type with the type-specific default value:
/// \code static inline PropertyTypeDecl<int> IntPropertyType = {IntPropertyTypeChangeSignalType};
///
/// To declare a property type with the a custom default value:
/// \code static inline PropertyTypeDecl<int> IntPropertyType = {IntPropertyTypeChangeSignalType, 10};
///
/// \note: An object can have only one property instance with a given type. Define multiple property
/// types with similar data type to have multiple properties with same data type. This applies also
/// to the property change signal types.
class MOX_API PropertyType
{
public:
    /// The change signal emitted when the property value is changed. You must declare the signal type before
    /// declaring the property.
    const SignalType& ChangedSignalType;

    /// Destructor.
    virtual ~PropertyType();

    /// Returns the access type of the property.
    /// \return The access type of the property.
    /// \see PropertyAccess
    PropertyAccess getAccess() const;

    /// Returns the type of the property.
    const VariantDescriptor& getValueType() const;

    /// Returns the default value of the property type.
    /// \return The default value.
    Variant getDefault() const;

protected:
    /// Constructor, creates a property type.
    /// \param typeDes The variant type descriptor of the type.
    /// \param signalType The reference to the signal type defining the change signal of the property.
    /// \param access The access type of the property.
    /// \param defaultValue The default value of the property type.
    PropertyType(VariantDescriptor&& typeDes, PropertyAccess access, const SignalType& signal, PropertyDataProviderInterface& defaultValue);

    /// The type descriptor of the property.
    VariantDescriptor m_typeDescriptor;
    /// The default value of the property type.
    PropertyDataProviderInterface& m_defaultValue;
    /// The property access type.
    PropertyAccess m_access = PropertyAccess::ReadWrite;
};

/// Property declarator template. Use this to define the property types.
/// \tparam ValueType The value type of the property.
/// \tparam access The access value of the property.
template <typename ValueType, PropertyAccess access>
class PropertyTypeDecl : private PropertyDefaultValue<ValueType>, public PropertyType
{
    DISABLE_COPY_OR_MOVE(PropertyTypeDecl)
public:
    /// Constructor.
    PropertyTypeDecl(const SignalType& sigChanged, const ValueType& defaultValue = ValueType())
        : PropertyDefaultValue<ValueType>(defaultValue)
        , PropertyType(VariantDescriptor::get<ValueType>(), access, sigChanged, *this)
    {
    }
};

} //namespace mox

#endif // PROPERTY_TYPE_HPP
