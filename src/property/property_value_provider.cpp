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

AbstractPropertyValueProvider::~AbstractPropertyValueProvider()
{
}

void AbstractPropertyValueProvider::attach(Property& property)
{
    throwIf<ExceptionType::ValueProviderAlreadyAttached>(isAttached());

    m_property = &property;
    m_property->attachValueProvider(shared_from_this());

    onAttached();

    activate();
}

void AbstractPropertyValueProvider::detach()
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());

    if (isActive())
    {
        deactivate();
    }

    onDetached();

    // Make sure that this is deleted last.
    auto keepAlive = shared_from_this();
    m_property->detachValueProvider(keepAlive);
    m_property = nullptr;
}

bool AbstractPropertyValueProvider::isAttached() const
{
    return m_property != nullptr;
}

void AbstractPropertyValueProvider::activate()
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());
    throwIf<ExceptionType::ActiveValueProvider>(m_isActive);

    m_isActive = true;
    m_property->activateValueProvider(shared_from_this());

    onActivated();
}

void AbstractPropertyValueProvider::deactivate()
{
    throwIf<ExceptionType::ValueProviderNotAttached>(!isAttached());
    throwIf<ExceptionType::InactiveValueProvider>(!m_isActive);

    m_isActive = false;
    m_property->deactivateValueProvider(shared_from_this());

    onDeactivated();
}

bool AbstractPropertyValueProvider::isActive() const
{
    return m_isActive;
}

void AbstractPropertyValueProvider::set(const Variant& value)
{
    if (setLocalValue(value))
    {
        m_property->changed.activate(Callable::ArgumentPack(value));
    }
}

} // namespace mox

