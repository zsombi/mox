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

namespace mox
{

AbstractPropertyValueProvider::AbstractPropertyValueProvider(ValueProviderFlags flags)
    : m_flags(flags)
{
}

AbstractPropertyValueProvider::~AbstractPropertyValueProvider()
{
}

ValueProviderFlags AbstractPropertyValueProvider::getFlags() const
{
    return m_flags;
}

bool AbstractPropertyValueProvider::hasFlags(ValueProviderFlags flags) const
{
    return (m_flags & flags) == flags;
}

void AbstractPropertyValueProvider::attach(Property& property)
{
    throwIf<ExceptionType::ValueProviderAlreadyAttached>(isAttached());

    m_property = &property;
    m_property->addValueProvider(shared_from_this());

    onAttached();
}

void AbstractPropertyValueProvider::detach()
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());

    onDetached();

    // Make sure that this is deleted last.
    auto keepAlive = shared_from_this();
    m_property->removeValueProvider(keepAlive);
    m_property = nullptr;
}

bool AbstractPropertyValueProvider::isAttached() const
{
    return m_property != nullptr;
}

void AbstractPropertyValueProvider::update(const Variant& value)
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
    m_property->update(value);
}

} // namespace mox

