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

#ifndef BINDING_HPP
#define BINDING_HPP

#include <mox/property/property.hpp>
#include <mox/property/property_value_provider.hpp>

namespace mox
{

class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

/// Binding class. Provides the interface for property bindings.
class MOX_API Binding : public PropertyValueProvider
{
public:
    /// Evaluates the binding. This is a binding specific action that updates the target
    /// property of the binding.
    virtual void evaluate() = 0;    

    /// \name Binding functions
    /// \{
    /// Creates a property binding between a set of properties. If all the properties are writable, the binding
    /// is a two-way binding. If one of the properties is read-only, the binding is one way only, and it
    /// is attached only to the writable property. If there are more than one read-only properties specified,
    /// the binding fails. The order of the properties are taken so that the left one is the source of the right
    /// one.
    /// \param properties The list of properties to bind.
    /// \return The property binding handler on success, nullptr on failure.
    template <ValueProviderFlags Flags = ValueProviderFlags::Generic, class... Prop>
    static BindingSharedPtr create(Prop&... properties)
    {
        std::array<Property*, sizeof...(Prop)> props = {{&properties...}};
        return bindProperties(std::vector<Property*>(props.begin(), props.end()), Flags);
    }
    /// \}

protected:
    /// Constructor.
    explicit Binding(ValueProviderFlags flags = ValueProviderFlags::Generic);

    static BindingSharedPtr bindProperties(std::vector<Property*> properties, ValueProviderFlags flags);
};

} // mox

#endif // BINDING_HPP
