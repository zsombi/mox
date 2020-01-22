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

#include <unordered_set>

namespace mox
{

class BindingSubscriber;

class PropertyPrivate
{
public:
    DECLARE_PUBLIC(Property)

    static inline Property* current = nullptr;
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

    using SubscriberCollection = std::unordered_set<BindingSharedPtr>;

    SubscriberCollection bindingSubscribers;
    BindingSharedPtr bindingsHead;
    /// The p-object.
    Property* p_ptr;
    /// The property data.
    AbstractPropertyData& dataProvider;
    /// The type of the property.
    PropertyType* type = nullptr;
    /// The host object of the property.
    Instance host;

    /// Constructor.
    explicit PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, Instance host);

    void accessed();
    void notifyChanges();
    void clearAllSubscribers();
    void clearBindings();
    void removeNonPermanentBindings();

    void eraseBinding(Binding& binding);
    void addBinding(BindingSharedPtr binding);
    void activateBinding(Binding& binding);
};

}

#endif // PROPERTY_P_HPP
