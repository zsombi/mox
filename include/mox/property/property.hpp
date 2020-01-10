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

#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include <mox/config/platform_config.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/property/property_data.hpp>
#include <mox/property/property_type.hpp>
#include <mox/signal/signal.hpp>

#include <mox/property/property_value_provider.hpp>
#include <vector>

namespace mox
{

/// The Property class provides the primitives for the property handling in Mox. To declare a property
/// for your class, use PropertyDecl<> template.
///
/// Properties do not hold the value. They use value providers to maintain the default and actual values
/// of the property. These values of the value providers are called local values. Each property has a
/// default value provider which maintains the default value of the property.
///
/// A property can have multiple value providers, and only one of those can be active at a time, and
/// only the default value provider can reset the property to the default value.
/// \see PropertyDecl<>, PropertyValueProvider
struct PropertyPrivate;
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
    bool isReadOnly() const;

    /// Property getter, returns the property value as a variant. The property value is the local value
    /// of the active value provider.
    /// \return The property value as variant.
    Variant get() const;

    /// Property setter, sets the property value from a variant. When called, it silently detaches all
    /// the attached value providers, except the default value priovider, and sets the local value of
    /// the default value provider.
    void set(const Variant& value);

    /// Resets the property value to the default. When called, it silently detaches all the attached
    /// value providers but the default value provider, and resets the default value provider to the
    /// default value.
    void reset();

    /// Gets the default value provider of the property.
    PropertyValueProviderSharedPtr getDefaultValueProvider() const;
    /// Returns the property value provider that exclusively provides the property value.
    PropertyValueProviderSharedPtr getExclusiveValueProvider() const;

protected:
    /// Constructor.
    explicit Property(Instance host, PropertyType& type, AbstractPropertyData& data);

    /// Detaches the value providers which satisfy the flags.
    void detachValueProviders(ValueProviderFlags flags);

private:
    Property() = delete;
    DISABLE_COPY_OR_MOVE(Property)
};

/// Declare a property in your class using this class. The template defines the default value provider
/// for the property. You can use this value provider to derive your default value providers, or any
/// value provider for the property.
/// \tparam ValueType The value type of the property.
template <typename ValueType>
class PropertyDecl : protected PropertyData<ValueType>, public Property
{
    using DataType = PropertyData<ValueType>;
    using ThisType = PropertyDecl<ValueType>;

public:

    /// Construct a property with a host, type and default value. Create read-write properties with
    /// this constructor.
    /// \tparam PropertyHost The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param defaultValue The default value of the property.
    template <class PropertyHost>
    explicit PropertyDecl(PropertyHost& host, PropertyType& type, const ValueType& defaultValue)
        : DataType(defaultValue)
        , Property(&host, type, *this)
    {
        if (!isReadOnly())
        {
            using VP = DefaultValueProvider<ValueType>;
            auto defvp = make_polymorphic_shared<PropertyValueProvider, VP>(defaultValue);
            defvp->attach(*this);
        }
    }

    /// Construct a property with a custom \a defaultValueProvider.
    /// \tparam PropertyHowt The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param defaultValueProvider The default value provider of the property.
    template <class PropertyHost, class DefValueProvider>
    explicit PropertyDecl(PropertyHost& host, PropertyType& type, std::shared_ptr<DefValueProvider> defaultValueProvider)
        : Property(&host, type, static_cast<AbstractPropertyData&>(*this))
    {
        throwIf<ExceptionType::MissingPropertyDefaultValueProvider>(!defaultValueProvider || !defaultValueProvider->hasFlags(ValueProviderFlags::Default));
        defaultValueProvider->attach(*this);
    }

    /// Cast opertator, the property getter.
    operator ValueType() const
    {
        return this->getValue();
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
