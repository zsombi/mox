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

class PropertyPrivate;

/// Abstract property data storage.
class MOX_API AbstractPropertyData
{
public:
    /// Destructor.
    virtual ~AbstractPropertyData() = default;

    /// Updates property data. If the new value differs from the current value, the method activates
    /// the change signal of the property, and notifies the bindings subscribed to receive changes on
    /// the property data.
    /// \param newValue The variant holding the value to update.
    void updateData(const Variant& newValue);

    virtual void initialize() {}

protected:
    /// Constructor.
    explicit AbstractPropertyData() = default;

    friend class PropertyPrivate;
    PropertyPrivate* m_property = nullptr;
};


/// Property data storage template.
/// \tparam ValueType The type of the data stored.
template <typename ValueType>
class PropertyData : public AbstractPropertyData
{
    ValueType m_value = ValueType();

    /// Constructor.
    PropertyData() = delete;

public:
    /// Constructs a property data from a value.
    PropertyData(ValueType v)
        : m_value(v)
    {
    }

    void initialize() override
    {
        updateData(Variant(m_value));
    }

};

} //mox

#endif // PROPERTY_DATA_HPP
