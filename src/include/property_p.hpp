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

    using SubscriberCollection = std::unordered_set<BindingSharedPtr>;

    /// The bindings subscribed for the property changes.
    SubscriberCollection bindingSubscribers;
    /// The list of bindings.
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
    /// Informs the proeprty being accessed.
    void notifyAccessed();
    /// Notifies the subscribers about the property value change.
    void notifyChanges();
    /// Clears the subscribers, and marks them all invalid.
    void clearAllSubscribers();
    /// Clears the bindings.
    void clearBindings();
    /// Removes detachable bindings.
    void removeDetachableBindings();

    /// Removes a binding from the list.
    void eraseBinding(Binding& binding);
    /// Adds a binding to the list.
    void addBinding(BindingSharedPtr binding);
    /// Activates a binding by moving the binding to the top of the list.
    void activateBinding(Binding& binding);
};

}

#endif // PROPERTY_P_HPP
