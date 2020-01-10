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

#ifndef PROPERTY_P_HPP
#define PROPERTY_P_HPP

#include <mox/property/property.hpp>
#include <mox/config/pimpl.hpp>

namespace mox
{

struct PropertyPrivate : PimplHelpers<Property, PropertyPrivate>
{
    DECLARE_PUBLIC(Property)
    thread_local static inline Property* current = nullptr;

    class Scope
    {
        Property* backup = nullptr;
    public:
        explicit Scope(Property& property)
            : backup(current)
        {
            current = &property;
        }
        ~Scope()
        {
            current = backup;
        }
    };

    PropertyValueProviderSharedPtr m_valueProviders;
    PropertyValueProviderSharedPtr m_defaultValueProvider;

    /// The p-object.
    Property* p_ptr;
    /// The property data.
    AbstractPropertyData& m_data;
    /// The type of the property.
    PropertyType* m_type = nullptr;
    /// The host object of the property.
    Instance m_host;
    /// Activation lock flag.
    bool m_activating = false;

    /// Constructor.
    explicit PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, Instance host);
    /// Attaches a value provider to the property. When attached, the value provider is appended to
    /// the value providers' list.
    void addValueProvider(PropertyValueProviderSharedPtr vp);
    /// Detaches a value provider from the property. When detached, the value provider is removed from
    /// the property.
    /// \return The value provider to enable, nullptr if no value provider has to be enabled.
    PropertyValueProviderSharedPtr removeValueProvider(PropertyValueProviderSharedPtr vp);

    /// Activates the value provider. The value provider must be set as enabled. The last enabled value provider
    /// is disabled.
    void activateValueProvider(PropertyValueProviderSharedPtr vp);

    /// Updates the value of the property.
    void update(const Variant& value);
};

}

#endif // PROPERTY_P_HPP
