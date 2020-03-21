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

#ifndef PROPERTY_BINDING_HPP
#define PROPERTY_BINDING_HPP

#include <mox/core/meta/property/binding/binding.hpp>

namespace mox
{

class PropertyBinding;
using PropertyBindingSharedPtr = std::shared_ptr<PropertyBinding>;

class PropertyBindingPrivate;
/// Property binding. The property binding is realized between two properties. The binding created is a one-way
/// binding. The binding is evaluated when the source property is changed. Use this to bind two properties to
/// each other.
///
/// Use BindingGroup to enable two-way binding.
class MOX_API PropertyBinding : public Binding
{
    DECLARE_PRIVATE(PropertyBindingPrivate)

public:
    /// Creates a property binding with a source. You must attach the binding to a target to evaluate.
    /// \param source The property binding source property, which provides the value for the parget.
    /// \param permanent If the binding is meant to survive write operation on target property, set to \e true.
    /// If the binding is meant to discard on target property write, set to \e false.
    /// \return The property binding created, in detached state.
    static PropertyBindingSharedPtr create(Property& source, bool permanent);

    /// Creates a permanent binding between a \a target and \a source property, and attaches the binding to
    /// the \a target. The binding survives property writes on target.
    /// \param target The target property.
    /// \param source The source property.
    /// \return The binding handler, nullptr if \a target or both \a target and \a source are read-only.
    static PropertyBindingSharedPtr bindPermanent(Property& target, Property& source);

    /// Creates an auto-detaching binding between a \a target and \a source property, and attaches the binding
    /// to the \a target. The binding is detached when write operation occurs on \a target.
    /// \param target The target property.
    /// \param source The source property.
    /// \return The binding handler, nullptr if \a target or both \a target and \a source are read-only. The
    /// binding is added to the target.
    static PropertyBindingSharedPtr bind(Property& target, Property& source);

protected:
    /// Constructor.
    explicit PropertyBinding(Property& source, bool isPermanent);

    void initialize();
    /// Evaluates property binding.
    void evaluate() override;
};

} // mox

#endif // PROPERTY_BINDING_HPP
