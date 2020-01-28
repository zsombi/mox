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

#include <mox/property/property.hpp>
#include <mox/config/error.hpp>

#include <binding_p.hpp>

namespace mox
{

PropertyBindingPrivate::PropertyBindingPrivate(PropertyBinding* pp, Property& source, bool permanent)
    : BindingPrivate(pp, permanent)
    , source(&source)
{
}


PropertyBinding::PropertyBinding(Property& source, bool permanent)
    : Binding(pimpl::make_d_ptr<PropertyBindingPrivate>(this, source, permanent))
{
}

void PropertyBinding::initialize()
{
    BindingScope setCurrent(*this);
    (void)d_func()->source->get();
}

void PropertyBinding::evaluate()
{
    if (!isEnabled() || !getTarget())
    {
        return;
    }

    auto value = d_func()->source->get();
    updateTarget(value);
}

PropertyBindingSharedPtr PropertyBinding::create(Property& source, bool permanent)
{
    auto binding = make_polymorphic_shared_ptr<Binding>(new PropertyBinding(source, permanent));
    binding->initialize();
    return binding;
}

PropertyBindingSharedPtr PropertyBinding::bindPermanent(Property &target, Property &source)
{
    if (target.isReadOnly())
    {
        return nullptr;
    }

    auto binding = PropertyBinding::create(source, true);
    target.addBinding(binding);
    return binding;
}

PropertyBindingSharedPtr PropertyBinding::bindAutoDiscard(Property &target, Property &source)
{
    if (target.isReadOnly())
    {
        return nullptr;
    }

    auto binding = PropertyBinding::create(source, false);
    target.addBinding(binding);
    return binding;
}

} // namespace mox

