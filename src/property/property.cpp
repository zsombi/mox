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

#include "../signal/signal_p.h"
#include <mox/property/property.hpp>
#include <mox/config/error.hpp>

namespace mox
{

Property::Property(intptr_t host, PropertyType& type)
    : SharedLock<ObjectLock>(*reinterpret_cast<ObjectLock*>(host))
    , changed(host, type.getChangedSignalType())
    , m_type(&type)
    , m_host(host)
{
    m_type->addPropertyInstance(*this);
}

Property::~Property()
{
    // Detach the value providers.
    detachValueProviders();

    // Detach the default one too.
    FlagScope<true> lock(m_silentMode);
    m_valueProviders.front()->detach();

    m_type->removePropertyInstance(*this);
}

bool Property::isReadOnly() const
{
    return m_type->getAccess() == PropertyAccess::ReadOnly;
}

Variant Property::get() const
{
    lock_guard lock(*const_cast<Property*>(this));
    auto vp = getActiveValueProvider();
    throwIf<ExceptionType::MissingPropertyDefaultValueProvider>(!vp);
    return vp->getLocalValue();
}

void Property::set(const Variant& value)
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    auto vp = getDefaultValueProvider();
    detachValueProviders();
    if (!vp->isActive())
    {
        vp->activate();
    }

    vp->set(value);
}

void Property::reset()
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    auto vp = getDefaultValueProvider();
    detachValueProviders();
    if (!vp->isActive())
    {
        vp->activate();
    }

    vp->resetToDefault();
}

PropertyValueProviderSharedPtr Property::getDefaultValueProvider() const
{
    return m_valueProviders.front();
}

PropertyValueProviderSharedPtr Property::getActiveValueProvider() const
{
    return m_activeValueProvider >= 0 ? m_valueProviders[size_t(m_activeValueProvider)] : nullptr;
}


void Property::attachValueProvider(PropertyValueProviderSharedPtr vp)
{
    lock_guard lock(*this);
    if (isReadOnly())
    {
        FATAL(m_valueProviders.empty(), "Only default value provider is allowed on read-only properties")
    }

    m_valueProviders.push_back(vp);
}

void Property::detachValueProvider(PropertyValueProviderSharedPtr vp)
{
    lock_guard lock(*this);
    auto activeVP = getActiveValueProvider();

    mox::erase(m_valueProviders, vp);

    auto it = std::find(m_valueProviders.begin(), m_valueProviders.end(), activeVP);
    m_activeValueProvider = (it != m_valueProviders.end())
            ? int(std::distance(m_valueProviders.begin(), it))
            : -1;
}

void Property::activateValueProvider(PropertyValueProviderSharedPtr vp)
{
    if (m_activating || m_silentMode)
    {
        return;
    }

    lock_guard lock(*this);
    FlagScope<true> activeLock(m_activating);
    auto activeVp = getActiveValueProvider();

    Variant oldValue;
    if (activeVp)
    {
        oldValue = activeVp->getLocalValue();
    }
    if (activeVp && activeVp != vp && activeVp->isActive())
    {
        activeVp->deactivate();
    }

    auto it = std::find(m_valueProviders.begin(), m_valueProviders.end(), vp);
    if (it != m_valueProviders.end())
    {
        m_activeValueProvider = int(std::distance(m_valueProviders.begin(), it));
    }

    ScopeRelock unlock(*this);
    Variant newValue = get();
    if (oldValue != newValue)
    {
        changed.activate(Callable::ArgumentPack(newValue));
    }
}

void Property::deactivateValueProvider(PropertyValueProviderSharedPtr vp)
{
    if (m_activating || m_silentMode)
    {
        return;
    }

    FlagScope<true> activeLock(m_activating);

    Variant oldValue = get();

    {
        lock_guard lock(*this);
        m_activeValueProvider = m_valueProviders.empty() ? -1 : 0;

        if (m_valueProviders.front() != vp && !m_valueProviders.front()->isActive())
        {
            m_valueProviders.front()->activate();
        }
    }

    Variant newValue = get();
    if (oldValue != newValue)
    {
        changed.activate(Callable::ArgumentPack(newValue));
    }
}

void Property::detachValueProviders()
{
    FlagScope<true> silentLock(m_silentMode);
    while (m_valueProviders.size() > 1)
    {
        m_valueProviders.back()->detach();
    }
    if (!m_valueProviders.front()->isActive())
    {
        m_valueProviders.front()->activate();
    }
    m_activeValueProvider = 0;
}

}
