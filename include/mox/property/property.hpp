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
#include <mox/property/property_data.hpp>
#include <mox/property/property_type.hpp>
#include <mox/signal/signal.hpp>

namespace mox
{

class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

/// The Property class provides the primitives for the property handling in Mox.
///
/// Mox provides two types of properties: read-write, or writable properties, and read-only properties.
/// Each property type has a change signal included, which reports value changes of the property.
///
/// Properties store the value using AbstractPropertyData. For read-only properties, you can specify
/// your own property data provider so you can update the property value. Calling the setter on read-only
/// properties throws exception.
///
/// You can change the value of a writable property by calling the setter of the property, or by adding
/// bindings to the property. Bindings provide automatic property value updates. A property can hold
/// several bindings, but only one of those is enabled at a time.
/// \see WritableProperty<>, ReadOnlyProperty<>, Binding, PropertyBinding, ExpressionBinding
class PropertyPrivate;
class MOX_API Property : public SharedLock<ObjectLock>
{
    DECLARE_PRIVATE_PTR(Property)

public:
    /// The change signal of the property. The signal is automatically emitted whenever a property
    /// value is changed.
    Signal changed;

    /// Destructor.
    virtual ~Property();

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

    /// Bindings
    /// \{
    /// Adds a binding to the property. The binding becomes the active binding of the property.
    /// \property binding The binding to add.
    void addBinding(BindingSharedPtr binding);

    /// Removes the binding from the property. If the binding was the active binding of the property,
    /// the next binding in the binding stack is activated.
    /// \param binding The binding to remove.
    void removeBinding(Binding& binding);

    /// Returns the current binding of the property.
    /// \return the current binding, nullptr if the property has no bindings.
    BindingSharedPtr getCurrentBinding();
    /// \}

protected:
    /// Constructor.
    explicit Property(Instance host, PropertyType& type, AbstractPropertyData& data);

    /// Returns the data provider of the property.
    AbstractPropertyData* getDataProvider() const;

    /// Notifies avout a property accessing.
    void notifyAccessed() const;

private:
    Property() = delete;
    DISABLE_COPY_OR_MOVE(Property)
};

/// Declare a read-only property in your class using this template class. You must define the property
/// data provider which you can use to update the value of the property. Derive the property data provider
/// using PropertyData<> template.
template <typename ValueType>
class ReadOnlyProperty : public Property
{
    using DataType = PropertyData<ValueType>;
    using ThisType = ReadOnlyProperty<ValueType>;

public:
    /// Constructs the property with the host, type and data provider. The data provider holds the default
    /// value of the property.
    /// \tparam PropertyHost The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param dataProvider The default value of the property.
    template <class PropertyHost, class DataProvider>
    explicit ReadOnlyProperty(PropertyHost& host, PropertyType& type, DataProvider& dataProvider)
        : Property(&host, type, dataProvider)
    {
        static_assert(std::is_base_of_v<DataType, DataProvider>, "The data provider must be derived from PropertyData<>");
    }

    /// Cast opertator, the property getter.
    operator ValueType() const
    {
        notifyAccessed();
        auto data = static_cast<DataType*>(getDataProvider());
        return data->getValue();
    }
};

/// Declare a writable property in your class using this template class. The template defines the property
/// data provider for the property. You can use this value provider to derive your default value providers, or any
/// value provider for the property.
/// \tparam ValueType The value type of the property.
template <typename ValueType>
class WritableProperty : protected PropertyData<ValueType>, public Property
{
    using DataType = PropertyData<ValueType>;
    using ThisType = WritableProperty<ValueType>;

public:

    /// Construct a property with a host, type and default value.
    /// \tparam PropertyHost The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param defaultValue The default value of the property.
    template <class PropertyHost>
    explicit WritableProperty(PropertyHost& host, PropertyType& type, const ValueType& defaultValue = ValueType())
        : DataType(defaultValue)
        , Property(&host, type, *this)
    {
    }

    /// Cast opertator, the property getter.
    operator ValueType() const
    {
        notifyAccessed();
        return DataType::getValue();
    }
    /// Property setter.
    /// \param value The value to set.
    auto& operator=(ValueType value)
    {
        set(Variant(value));
        return *this;
    }

    /// Copy operator.
    auto& operator=(const ThisType& other)
    {
        set(other.get());
        return *this;
    }

    /// Prefix increment operator.
    auto& operator++()
    {
        static_assert (std::is_integral_v<ValueType>, "Increment operator is available for integral types");
        auto value = this->getValue();
        set(Variant(++value));
        return *this;
    }

    /// Prefix decrement operator.
    auto& operator--()
    {
        static_assert (std::is_integral_v<ValueType>, "Increment operator is available for integral types");
        auto value = this->getValue();
        set(Variant(--value));
        return *this;
    }
};

}

#endif // PROPERTY_HPP
