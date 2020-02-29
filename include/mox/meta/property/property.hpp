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

#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include <mox/config/platform_config.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/meta/property/property_data.hpp>
#include <mox/meta/property/property_type.hpp>
#include <mox/meta/signal/signal.hpp>

namespace mox
{

class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

/// The Property class provides the primitives for the property handling in Mox.
///
/// Mox provides two types of properties: read-write, or writable properties, and read-only properties.
/// Each property type has a change signal included, which reports value changes of the property.
///
/// The property storage is provided by an AbstractPropertyData derived class. Such derivate is simply called
/// property data provider, and Mox provides a templated default implementation you can use to maintain
/// the data of your property. This property data provider can also update the read-only property value
/// you declare on a class.
///
/// You can change the value of a writable property by calling the setter of the property, or by adding
/// bindings to the property. Calling the setter on a read-only property asserts.
///
/// Bindings provide automatic property value updates. A property can hold several bindings, but only one
/// of those is enabled at a time.
class PropertyPrivate;
class MOX_API Property : public SharedLock
{
    DECLARE_PRIVATE_PTR(Property)

public:
    /// The change signal of the property. The signal is automatically emitted whenever a property
    /// value is changed.
    Signal changed;

    /// Constructs the property with the host, type and data provider. The data provider holds the default
    /// value of the property.
    /// \param host The host of the property.
    /// \param type The type of the property defined by a PropertyType instance.
    /// \param dataProvider The default value of the property.
    Property(ObjectLock& host, PropertyType& type, AbstractPropertyData& data);

    /// Destructor.
    ~Property();

    /// Checks the validity of a property. A property is valid if its type is set.
    /// \return If the property is valid, returns \e true, otherwise \e false.
    bool isValid() const;

    /// Returns the read-only state of the property.
    /// \return The read-only state of the property.
    bool isReadOnly() const;

    /// Property getter, returns the property value as a variant.
    /// \return The property value as variant.
    Variant get() const;

    /// Property setter, sets the property value from a variant. Removes the discardable bindings.
    /// \param value The property value to set.
    void set(const Variant& value);

    /// Resets the property value to the default. All property bindings are removed.
    void reset();

    /// Returns the current binding of the property.
    /// \return the current binding, nullptr if the property has no bindings.
    BindingSharedPtr getCurrentBinding();

    /// Cast opertator, the property getter.
    template <typename ValueType>
    operator ValueType() const
    {
        return static_cast<ValueType>(get());
    }

    /// Property setter.
    /// \param value The value to set.
    template <typename ValueType>
    void operator=(ValueType value)
    {
        set(Variant(value));
    }

private:
    Property() = delete;
    DISABLE_COPY_OR_MOVE(Property)
};

}

#endif // PROPERTY_HPP
