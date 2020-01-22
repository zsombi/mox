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

#ifndef BINDING_P_HPP
#define BINDING_P_HPP

#include <mox/property/property.hpp>
#include <mox/binding/binding.hpp>
#include <mox/binding/property_binding.hpp>
#include <mox/config/pimpl.hpp>

#include <unordered_set>

namespace mox
{

enum class BindingState
{
    Attaching,
    Attached,
    Detaching,
    Detached
};

class BindingPrivate
{
public:
    DECLARE_PUBLIC_PTR(Binding)

    using Collection = std::unordered_set<Property*>;

    Collection dependencies;
    BindingSharedPtr next;
    BindingSharedPtr prev;
    BindingGroupSharedPtr group;
    Property* target = nullptr;
    BindingState state = BindingState::Detached;
    bool enabled = false;
    bool evaluateOnEnabled = true;
    bool isPermanent = false;

    explicit BindingPrivate(Binding* pp, bool permanent);
    ~BindingPrivate();

    void attachToTarget(Property& target);
    void detachFromTarget();
    void addDependency(Property& dependency);
    void removeDependency(Property& dependency);
    void clearDependencies();
    void evaluateBinding();
};

class PropertyBindingPrivate : public BindingPrivate
{
public:
    DECLARE_PUBLIC_PTR(PropertyBinding)

    explicit PropertyBindingPrivate(PropertyBinding* pp, Property& source, bool permanent);

    Property* source = nullptr;
};

}

#endif // BINDING_P_HPP
