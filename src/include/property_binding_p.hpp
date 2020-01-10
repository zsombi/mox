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

#ifndef PROPERTY_BINDING_P_HPP
#define PROPERTY_BINDING_P_HPP

#include <mox/binding/binding.hpp>

namespace mox
{

/// Implements a property binding between two properties.
class PropertyBinding : public Binding
{
public:
    /// Constructor.
    explicit PropertyBinding(ValueProviderFlags flags, Property& source);

    void link(PropertyBinding& binding);
    void unlink(PropertyBinding& binding);

    /// Override of Binding::evaluate().
    void evaluate() override;

protected:
    /// Override of PropertyValueProvider::onAttached().
    void onAttached() override;

    /// Override of PropertyValueProvider::onDetached().
    void onDetached() override;

    /// Override of PropertyValueProvider::onEnabledChanged().
    void onEnabledChanged() override;

    class Links
    {
        std::array<PropertyBinding*, 2> bindings;

    public:
        explicit Links();
        bool link(PropertyBinding& binding);
        bool unlink(PropertyBinding& binding);
        void reset();
        bool empty() const;
    };

    Property* m_source = nullptr;
    Links m_linkedBindings;
    Signal::ConnectionSharedPtr m_connection;
};

}

#endif // PROPERTY_BINDING_P_HPP
