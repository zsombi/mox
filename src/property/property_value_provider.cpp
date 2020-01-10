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

#include <mox/property/property_value_provider.hpp>
#include <mox/property/property.hpp>
#include <mox/config/error.hpp>

#include <property_p.hpp>

namespace mox
{

static ValueProviderFlags fixFlags(ValueProviderFlags flags)
{
    if ((flags & ValueProviderFlags::Exclusive) == ValueProviderFlags::Exclusive)
    {
        return flags | ValueProviderFlags::Default | ValueProviderFlags::KeepOnWrite;
    }
    return flags;
}

PropertyValueProvider::PropertyValueProvider(ValueProviderFlags flags)
    : m_flags(fixFlags(flags))
{
}

PropertyValueProvider::~PropertyValueProvider()
{
}

ValueProviderFlags PropertyValueProvider::getFlags() const
{
    return m_flags;
}

bool PropertyValueProvider::hasFlags(ValueProviderFlags flags) const
{
    return (m_flags & flags) == flags;
}

void PropertyValueProvider::attach(Property& property)
{
    throwIf<ExceptionType::ValueProviderAlreadyAttached>(m_state == Attached);

    if (m_state == Attaching)
    {
        return;
    }

    m_state = Attaching;

    PropertyPrivate::get(property)->addValueProvider(shared_from_this());
    m_property = &property;

    onAttached();

    m_state = Attached;

    // Note: this may fail if the property has exclusive VPs.
    activate();
}

void PropertyValueProvider::detach()
{
    throwIf<ExceptionType::ValueProviderNotAttached>(m_state == Detached);

    if (m_state == Detaching)
    {
        return;
    }
    m_state = Detaching;

    auto dProperty = PropertyPrivate::get(*m_property);

    onDetached();

    // Make sure that this is deleted last.
    auto keepAlive = shared_from_this();
    auto nextVP = dProperty->removeValueProvider(keepAlive);
    m_property = nullptr;
    m_state = Detached;

    if (nextVP)
    {
        nextVP->setEnabled(true);
    }
}

bool PropertyValueProvider::isAttached() const
{
    return m_property != nullptr;
}

bool PropertyValueProvider::isEnabled() const
{
    return m_enabled;
}

void PropertyValueProvider::setEnabled(bool enabled)
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());

    if (m_enabled == enabled)
    {
        return;
    }

    m_enabled = enabled;

    if (m_enabled && m_property)
    {
        PropertyPrivate::get(*m_property)->activateValueProvider(shared_from_this());
    }

    onEnabledChanged();
}

Variant PropertyValueProvider::getLocalValue() const
{
    FATAL(false, "Only default property valu eproviders can have local values.")
    return Variant();
}

void PropertyValueProvider::activate()
{
    // Try to enable the value provider. If enabling is possible, call onActivating()
    // so value provider implementation can prepare the initial value to set to the
    // attached property.
    auto exclusive = m_property->getExclusiveValueProvider();
    if (exclusive && exclusive.get() != this)
    {
        return;
    }

    onActivating();

    setEnabled(true);
}

void PropertyValueProvider::update(const Variant& value)
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());
    // If the property is read-only, only default value provider can update its value.
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(m_property->isReadOnly() && !hasFlags(ValueProviderFlags::Default));
    // If the property has exclusive value providers, only that one can provide values for the property. Ignore the rest.
    auto exclusiveVp = m_property->getExclusiveValueProvider();
    if (exclusiveVp && (exclusiveVp.get() != this))
    {
        return;
    }
    PropertyPrivate::get(*m_property)->update(value);
}

} // namespace mox

