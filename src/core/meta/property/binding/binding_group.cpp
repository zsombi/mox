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

#include <private/binding_p.hpp>
#include <mox/core/meta/property/binding/binding_group.hpp>

namespace mox
{

/******************************************************************************
 * BindingGroup
 */
BindingGroup::~BindingGroup()
{
    detach();
}

void BindingGroup::addBinding(Binding& binding)
{

    m_bindings.push_back(binding.shared_from_this());
    auto dBinding = BindingPrivate::get(binding);
    dBinding->setGroup(shared_from_this());
}

void BindingGroup::removeBinding(Binding &binding)
{
    auto shBinding = binding.shared_from_this();
    erase(m_bindings, shBinding);

    if (m_normalizer && m_normalizer->getTarget() == shBinding)
    {
        m_normalizer.reset();
    }
    auto dBinding = BindingPrivate::get(binding);
    dBinding->setGroup(nullptr);
}

void BindingGroup::ungroupBindings()
{
    m_normalizer.reset();
    // remove the bindings from the group.
    for (auto binding : m_bindings)
    {
        BindingPrivate::get(*binding)->setGroup(nullptr);
    }
    m_bindings.clear();
}

void BindingGroup::detach()
{
    m_normalizer.reset();
    for (auto binding : m_bindings)
    {
        if (binding->isAttached())
        {
            binding->detach();
        }
    }
}

bool BindingGroup::isEmpty() const
{
    return m_bindings.empty();
}

size_t BindingGroup::getBindingCount() const
{
    return m_bindings.size();
}

BindingSharedPtr BindingGroup::at(size_t index)
{
    return m_bindings[index];
}

void BindingGroup::setNormalizer(Binding& targetBinding, BindingNormalizerPtr normalizer)
{
    // The target must be in the group.
    auto finder = [tgt = targetBinding.shared_from_this()](auto binding)
    {
        return binding == tgt;
    };
    auto it = std::find_if(m_bindings.cbegin(), m_bindings.cend(), finder);
    throwIf<ExceptionType::BindingNotInGroup>(it == m_bindings.cend());

    m_normalizer = std::move(normalizer);
    m_normalizer->m_target = targetBinding.shared_from_this();
}

BindingNormalizer* BindingGroup::getNormalizer() const
{
    return m_normalizer.get();
}

BindingGroupSharedPtr BindingGroup::bindProperties(const std::vector<Property*>& properties, bool permanent, bool circular)
{
    if (properties.empty())
    {
        return nullptr;
    }
    auto readOnly = (Property*)nullptr;

    for (auto property : properties)
    {
        if (property->isReadOnly())
        {
            if (readOnly)
            {
                // Cannot bind properties where we have more than one read-only property.
                return nullptr;
            }
            readOnly = property;
        }
    }

    BindingGroupSharedPtr group = create();

    if (readOnly)
    {
        // The read-only property is the source for all the other properties. The other properties are not bount.
        for (auto it = properties.rbegin(); it != properties.rend(); ++it)
        {
            auto property = *it;
            if (property == readOnly)
            {
                continue;
            }
            // Create a binding with the read-only property and add i to the group, as well as to the target property.
            auto binding = PropertyBinding::create(*readOnly, permanent);
            group->addBinding(*binding);
            binding->attach(*property);
        }
    }
    else
    {
        // Loop through the properties from the reverse side and bind them one by one. If the binding is cyclic,
        // bind the reverse last property (first in the list) to the reverse first (last in the list).
        auto binding = PropertyBindingSharedPtr();
        for (auto it = properties.rbegin(); it != properties.rend(); ++it)
        {
            auto property = *it;
            if (!binding)
            {
                // This is the first binding, so only create one now, without adding it to a target.
                binding = PropertyBinding::create(*property, permanent);
                continue;
            }
            // Add the binding to the target property, and after that to the group.
            binding->attach(*property);
            group->addBinding(*binding);

            // Create a new binding with the property.
            binding = PropertyBinding::create(*property, permanent);
        }
        // If the binding is circular (two-way), add the binding first property.
        if (circular)
        {
            binding->attach(*(*properties.rbegin()));
            group->addBinding(*binding);
        }
    }

    return group;
}

} // mox
