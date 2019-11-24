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
/// \see PropertyDecl<>, AbstractPropertyValueProvider
class MOX_API Property : public SharedLock<ObjectLock>
{
    friend class PropertyType;
    friend class AbstractPropertyValueProvider;

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

protected:
    /// Constructor.
    explicit Property(intptr_t owner, PropertyType& type);

    /// Gets the default value provider of the property.
    PropertyValueProviderSharedPtr getDefaultValueProvider() const;
    /// Gets the active value provider of the property.
    PropertyValueProviderSharedPtr getActiveValueProvider() const;

    /// Attaches a value provider to the property. When attached, the value provider is appended to
    /// the value providers' list.
    void attachValueProvider(PropertyValueProviderSharedPtr vp);
    /// Detaches a value provider from the property. When detached, the value provider is removed from
    /// the property.
    void detachValueProvider(PropertyValueProviderSharedPtr vp);
    /// Activates a value provider. When done so, the previous active value provider is deactivated.
    void activateValueProvider(PropertyValueProviderSharedPtr vp);
    /// Deactivates a value provider, and sets the default value provider as the active one.
    void deactivateValueProvider(PropertyValueProviderSharedPtr vp);
    /// Detaches all the value providers, except the default value provider.
    void detachValueProviders();

    /// The vector with the value providers of the property, including the default one.
    std::vector<PropertyValueProviderSharedPtr> m_valueProviders;
    /// The type of the property.
    PropertyType* m_type = nullptr;
    /// The host object of the property.
    intptr_t m_host = 0u;
    /// The index of the active value provider.
    int m_activeValueProvider = -1;
    /// Activation lock flag.
    bool m_activating = false;
    /// Silent mode lock flag.
    bool m_silentMode = false;

private:
    Property() = delete;
};

/// Declare a property in your class using this class. The template defines the default value provider
/// for the property. You can use this value provider to derive your default value providers, or any
/// value provider for the property.
/// \tparam ValueType The value type of the property.
template <typename ValueType>
class PropertyDecl : public Property
{
public:
    /// The default value provider of this property.
    class PropertyDefaultValueProvider : public AbstractPropertyValueProvider
    {
        ValueType m_value;
        ValueType m_defaultValue;

    public:
        /// Constructor, creates the value provider with a default value.
        PropertyDefaultValueProvider(ValueType defaultValue)
            : m_value(defaultValue)
            , m_defaultValue(defaultValue)
        {}

        /// Local value getter.
        Variant getLocalValue() const override
        {
            return Variant(m_value);
        }
        /// Local value setter.
        bool setLocalValue(const Variant& value) override
        {
            bool changed = (m_value != value);
            m_value = (ValueType)value;
            return changed;
        }
        /// Resets the value provider to the default value.
        void resetToDefault() override
        {
            set(Variant(m_defaultValue));
        }
    };
    using PropertyDefaultValueProviderSharedPtr = std::shared_ptr<PropertyDefaultValueProvider>;

    /// Construct a property with a host, type and default value. Create read-write properties with
    /// this constructor.
    /// \tparam PropertyHowt The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param defaultValue The default value of the property.
    template <class PropertyHost>
    explicit PropertyDecl(PropertyHost& host, PropertyType& type, const ValueType& defaultValue)
        : Property(reinterpret_cast<intptr_t>(&host), type)
    {
        if (!isReadOnly())
        {
            auto defvp = make_polymorphic_shared<AbstractPropertyValueProvider, PropertyDefaultValueProvider>(defaultValue);
            defvp->attach(*this);
        }
    }

    /// Construct a property with a custom \a defaultValueProvider.
    /// \tparam PropertyHowt The type of the class that hosts the property.
    /// \param host The host of the property.
    /// \param type The reference to the PropertyType that describes the property.
    /// \param defaultValueProvider The default value provider of the property.
    template <class PropertyHost>
    explicit PropertyDecl(PropertyHost& host, PropertyType& type, PropertyValueProviderSharedPtr defaultValueProvider)
        : Property(reinterpret_cast<intptr_t>(&host), type)
    {
        FATAL(defaultValueProvider, "Properties must have defaule value providers")
        defaultValueProvider->attach(*this);
    }

    /// Cast opertator, the property getter.
    operator ValueType()
    {
        return get();
    }
    /// Const cast opertator, the property getter.
    operator ValueType() const
    {
        return get();
    }
    /// Property setter.
    /// \param value The value to set.
    auto& operator=(ValueType value)
    {
        set(Variant(value));
        return *this;
    }
};

}

#endif // PROPERTY_HPP
