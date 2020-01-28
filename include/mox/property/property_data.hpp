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

#ifndef PROPERTY_DATA_HPP
#define PROPERTY_DATA_HPP

#include <mox/config/platform_config.hpp>
#include <mox/metadata/variant.hpp>

namespace mox
{

class Property;

/// Abstract property data storage.
class MOX_API AbstractPropertyData
{
public:
    /// Destructor.
    virtual ~AbstractPropertyData() = default;

    /// Default data setter.
    /// \return The property default data stored in a variant.
    virtual void resetToDefault() = 0;

    /// Updates property data. If the new value differs from the current value, the method activates
    /// the change signal of the property, and notifies the bindings subscribed to receive changes on
    /// the property data.
    /// \param newValue The variant holding the value to update.
    void updateData(const Variant& newValue);

    virtual operator Variant() const = 0;

protected:
    /// Constructor.
    explicit AbstractPropertyData() = default;

    /// Generic data getter.
    /// \return The property data stored in a variant.
    virtual Variant getData() const = 0;

    /// Generic property data setter.
    /// \param value The property data as a variant.
    virtual void setData(const Variant& value) = 0;

    friend class Property;
    Property* m_property = nullptr;
};


/// Property data storage template.
/// \tparam ValueType The type of the data stored.
template <typename ValueType>
class PropertyData : public AbstractPropertyData
{
    ValueType m_value = ValueType();
    ValueType m_defaultValue = ValueType();

protected:
    /// The data setter.
    void setData(const Variant& value) override
    {
        m_value = (ValueType)value;
    }

    /// The data getter.
    Variant getData() const override
    {
        return Variant(m_value);
    }

    /// The default data getter.
    void resetToDefault() override
    {
        updateData(Variant(m_defaultValue));
    }

    operator Variant() const override
    {
        return getData();
    }

public:
    /// Constructor.
    PropertyData() = default;
    /// Constructs a property data from a value.
    PropertyData(ValueType v)
        : m_value(v)
        , m_defaultValue(v)
    {
    }

    /// Direct cast to value type.
    ValueType getValue() const
    {
        return m_value;
    }
};

} //mox

#endif // PROPERTY_DATA_HPP
