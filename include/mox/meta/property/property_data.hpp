/*
 * Copyright (C) 2017-2020 bitWelder
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

#ifndef PROPERTY_DATA_HPP
#define PROPERTY_DATA_HPP

#include <mox/config/platform_config.hpp>
#include <mox/meta/core/variant.hpp>

namespace mox
{

class PropertyStorage;

/// Declares the interface for the property data providers. These data providers serve the default value for
/// the property types and the value for the properties.
class MOX_API PropertyDataProviderInterface
{
public:
    /// Destructor.
    virtual ~PropertyDataProviderInterface() = default;
    /// Data getter.
    virtual Variant getData() const = 0;
    /// Data setter.
    virtual void setData(const Variant& value) = 0;
};

/// Property data provider.
class MOX_API PropertyDataProvider : public PropertyDataProviderInterface
{
    friend class PropertyStorage;
    PropertyStorage* m_property = nullptr;

protected:
    /// Constructor.
    explicit PropertyDataProvider() = default;

    /// Updates property data. If the new value differs from the current value, the method activates
    /// the change signal of the property, and notifies the bindings subscribed to receive changes on
    /// the property data.
    /// \param newValue The variant holding the value to update.
    void update(const Variant& newValue);

public:
    /// Destructor.
    virtual ~PropertyDataProvider() = default;
};

/// Template class, provides the default value storage for a proeprty type.
template <typename ValueType>
class PropertyDefaultValue : public PropertyDataProviderInterface
{
    ValueType m_defaultValue;
public:
    PropertyDefaultValue(ValueType value)
        : m_defaultValue(value)
    {}

    Variant getData() const override
    {
        return Variant(m_defaultValue);
    }
    void setData(const Variant&) override
    {
        FATAL(false, "You cannot change default value of a property type.");
    }
};

/// Property data provider template. Stores the data of the property
/// \tparam ValueType The type of the data stored.
template <typename ValueType>
class PropertyData : public PropertyDataProvider
{
    ValueType m_value = ValueType();

    /// Constructor.
    PropertyData() = delete;

protected:
    Variant getData() const override
    {
        return Variant(m_value);
    }
    void setData(const Variant &value) override
    {
        m_value = static_cast<ValueType>(value);
    }

public:
    /// Constructs a property data from a value.
    PropertyData(const ValueType& v = ValueType())
        : m_value(v)
    {
    }
};

} //mox

#endif // PROPERTY_DATA_HPP
