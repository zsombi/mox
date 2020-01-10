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

#include <mox/binding/binding.hpp>

#include <property_binding_p.hpp>

namespace mox
{

Binding::Binding(ValueProviderFlags flags)
    : PropertyValueProvider(flags)
{
}

BindingSharedPtr Binding::bindProperties(std::vector<Property*> properties, ValueProviderFlags flags)
{
    Property* readOnlyProperty = nullptr;
    for (auto property : properties)
    {
        if (property->isReadOnly())
        {
            if (readOnlyProperty)
            {
                // Cannot have more than one read-only property.
                return nullptr;
            }
            readOnlyProperty = property;
        }
    }

    if (readOnlyProperty)
    {
        // The read-only property drives the binding, and the binding is one-way.
        auto binding = make_polymorphic_shared<Binding, PropertyBinding>(flags, *readOnlyProperty);
        auto pb = binding;
        // All the other bindings are linked.
        for (auto it = properties.begin(); it != properties.end(); ++it)
        {
            auto property = *it;
            if (property == readOnlyProperty)
            {
                continue;
            }
            pb->attach(*property);
            // Create the next binding, until we reach the last property.
            if ((it + 1) != properties.end())
            {
                auto next = make_polymorphic_shared<Binding, PropertyBinding>(flags, *property);
                pb->link(*next);
                pb = next;
            }
        }

        return binding;
    }
    else
    {
        // Link all the bindings, and loop the last binding to the first.
        auto it = properties.begin();
        auto binding = make_polymorphic_shared<Binding, PropertyBinding>(flags, **it);
        auto pb = binding;
        for (++it; it != properties.end(); ++it)
        {
            auto property = *it;
            pb->attach(*property);
            // Create the next binding.
            auto next = make_polymorphic_shared<Binding, PropertyBinding>(flags, *property);
            pb->link(*next);
            pb = next;
        }
        // Attach the last binding to the first property.
        pb->attach(**properties.begin());

        return binding;
    }
}


PropertyBinding::Links::Links()
    : bindings({nullptr, nullptr})
{
}

bool PropertyBinding::Links::link(PropertyBinding& binding)
{
    if (std::find(bindings.begin(), bindings.end(), &binding) != bindings.end())
    {
        return false;
    }

    if (!bindings[0])
    {
        bindings[0] = &binding;
    }
    else if (!bindings[1])
    {
        bindings[1] = &binding;
    }
    else
    {
        FATAL(false, "Attempt adding the 3rd link to a property binding.")
    }

    return true;
}

bool PropertyBinding::Links::unlink(PropertyBinding& binding)
{
    auto it = std::find(bindings.begin(), bindings.end(), &binding);
    if (it == bindings.end())
    {
        return false;
    }
    *it = &binding;
    return true;
}

void PropertyBinding::Links::reset()
{
    for (auto binding : bindings)
    {
        if (binding)
        {
            binding->detach();
        }
    }
    bindings.fill(nullptr);
}

bool PropertyBinding::Links::empty() const
{
    return std::find_if(bindings.begin(), bindings.end(), [](auto binding) { return binding != nullptr; }) == bindings.end();
}


PropertyBinding::PropertyBinding(ValueProviderFlags flags, Property& source)
    : Binding(flags)
    , m_source(&source)
{
}

void PropertyBinding::link(PropertyBinding& binding)
{
    if (m_linkedBindings.link(binding))
    {
        binding.link(*this);
    }
}

void PropertyBinding::unlink(PropertyBinding& binding)
{
    if (m_linkedBindings.unlink(binding))
    {
        binding.unlink(*this);
    }
}

void PropertyBinding::onAttached()
{
    m_connection = m_source->changed.connect(*this, &PropertyBinding::evaluate);
    FATAL(m_connection, "Binding connection failed.")
}

void PropertyBinding::onDetached()
{
    if (m_connection)
    {
        m_connection->disconnect();
        m_connection.reset();
    }

    if (!m_linkedBindings.empty())
    {
        m_linkedBindings.reset();
    }
}

void PropertyBinding::onEnabledChanged()
{
    evaluate();
}

void PropertyBinding::evaluate()
{
    if (!isEnabled())
    {
        return;
    }
    update(m_source->get());
}

} // mox
