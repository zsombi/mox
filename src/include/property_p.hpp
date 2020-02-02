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

class PropertyPrivate
{
    using SubscriberCollection = std::unordered_set<BindingSharedPtr>;
    using BindingCollection = std::vector<BindingSharedPtr>;

    /// The bindings subscribed for the property changes.
    SubscriberCollection bindingSubscribers;
    /// The list of bindings.
    BindingCollection bindings;
    /// The p-object.
    Property* p_ptr;
    /// The property data.
    AbstractPropertyData& dataProvider;
    /// The type of the property.
    PropertyType* type = nullptr;
    /// The host object of the property.
    Instance host;

public:
    DECLARE_PUBLIC(Property)

    /// Constructor.
    explicit PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, Instance host);
    ~PropertyPrivate();

    /// Thread-safe functions.
    /// Thread-safe. Adds a binding to the property. Called by Binding::attach().
    void addBinding(BindingSharedPtr binding);
    /// Thread-safe. Removes the binding from the property. If the binding was the active binding of the property,
    /// the next binding in the binding stack is activated.
    /// \param binding The binding to remove.
    void removeBinding(Binding& binding);
    /// Tries to activate the binding on head.
    void tryActivateHeadBinding();
    /// Activates a binding by moving the binding to the top of the list.
    void activateBinding(Binding& binding);
    /// Updates the property data. Doe snot
    void updateData(const Variant& value);
    /// Unsubscribes a binding from the property.
    void unsubscribe(BindingSharedPtr binding);

    /// Non thread-safe.
    Variant fetchDataUnsafe() const;
    inline const Instance& getHost() const
    {
        return host;
    }

private:
    /// Clears the bindings.
    void clearBindings();

    /// Non thread-safe functions.
    /// Informs the proeprty being accessed.
    void notifyAccessed();
    /// Notifies the subscribers about the property value change.
    void notifyChanges();
};

}

#endif // PROPERTY_P_HPP
