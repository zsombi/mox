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

Property::Property(Instance host, PropertyType& type, AbstractPropertyData& data)
    : SharedLock<ObjectLock>(*host.as<ObjectLock>())
    , changed(host, type.getChangedSignalType())
    , m_data(data)
    , m_type(&type)
    , m_host(host)
{
    m_type->addPropertyInstance(*this);
}

Property::~Property()
{
    // Detach the value providers.
    lock_guard lockVp(m_valueProviders);
    auto detacher = [](auto vp)
    {
        if (vp) vp->detach();
    };
    m_valueProviders.forEach(detacher);

    m_type->removePropertyInstance(*this);
}

bool Property::isReadOnly() const
{
    return m_type->getAccess() == PropertyAccess::ReadOnly;
}

Variant Property::get() const
{
    return m_data.getData();
}

void Property::set(const Variant& value)
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    // Detach only generic value providers on property write.
    detachValueProviders(ValueProviderFlags::Generic);

    auto exclusiveVp = getExclusiveValueProvider();
    if (exclusiveVp)
    {
        return;
    }
    // Set the value.
    update(value);
}

void Property::update(const Variant &value)
{
    if (value == m_data.getData())
    {
        return;
    }
    m_data.setData(value);
    changed.activate(Callable::ArgumentPack(value));
}

void Property::reset()
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    detachValueProviders(ValueProviderFlags::KeepOnWrite);
    detachValueProviders(ValueProviderFlags::Exclusive);

    auto defaultVp = getDefaultValueProvider();
    throwIf<ExceptionType::MissingPropertyDefaultValueProvider>(!defaultVp);
    set(defaultVp->getLocalValue());
}

PropertyValueProviderSharedPtr Property::getDefaultValueProvider() const
{
    auto findDefaultVP = [](auto& vp)
    {
        return vp && vp->hasFlags(ValueProviderFlags::Default);
    };
    auto idx = m_valueProviders.findIf(findDefaultVP);
    if (idx)
    {
        return m_valueProviders[*idx];
    }
    return nullptr;
}

PropertyValueProviderSharedPtr Property::getExclusiveValueProvider() const
{
    auto findDefaultVP = [](auto& vp)
    {
        return (vp && vp->hasFlags(ValueProviderFlags::Exclusive));
    };
    auto idx = m_valueProviders.findIf(findDefaultVP);
    if (idx)
    {
        return m_valueProviders[*idx];
    }
    return nullptr;
}

void Property::addValueProvider(PropertyValueProviderSharedPtr vp)
{
    throwIf<ExceptionType::PropertyHasDefaultValueProvider>(getDefaultValueProvider() && vp && vp->hasFlags(ValueProviderFlags::Default));
    lock_guard lock(*this);
    if (isReadOnly())
    {
        FATAL(m_valueProviders.empty(), "Only default value provider is allowed on read-only properties")
    }

    lock_guard vpLock(m_valueProviders);
    m_valueProviders.append(vp);
}

void Property::removeValueProvider(PropertyValueProviderSharedPtr vp)
{
    lock_guard lock(*this);
    lock_guard lockVp(m_valueProviders);
    auto idx = m_valueProviders.find(vp);
    if (idx)
    {
        m_valueProviders[*idx].reset();
    }
}

void Property::detachValueProviders(ValueProviderFlags flags)
{
    FlagScope<true> silentLock(m_silentMode);

    lock_guard vpLock(m_valueProviders);
    auto detacher = [&flags](auto vp)
    {
        if (vp->getFlags() == flags)
        {
            vp->detach();
        }
    };
    m_valueProviders.forEach(detacher);
}

}
