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
#include <property_p.hpp>
#include <mox/property/property.hpp>
#include <mox/config/error.hpp>

namespace mox
{

PropertyPrivate::PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, Instance host)
    : p_ptr(&p)
    , m_data(data)
    , m_type(&type)
    , m_host(host)
{
}

void PropertyPrivate::addValueProvider(PropertyValueProviderSharedPtr vp)
{
    FATAL(vp, "Null property value provider.")

    P_PTR(Property);
    // Only one default value provider is allowed. A second default VP is allowed only if it is exclusive too.
    throwIf<ExceptionType::PropertyHasDefaultValueProvider>(p->getDefaultValueProvider() && vp->hasFlags(ValueProviderFlags::Default) && !vp->hasFlags(ValueProviderFlags::Exclusive));
    // Only one exclusive value provider is allowed.
    throwIf<ExceptionType::PropertyHasExclusiveValueProvider>(vp->hasFlags(ValueProviderFlags::Exclusive) && p->getExclusiveValueProvider());

    lock_guard lock(*p);
    if (p->isReadOnly())
    {
        FATAL(!m_valueProviders, "Only default value provider is allowed on read-only properties")
    }

    bool isDefaultVP = vp->hasFlags(ValueProviderFlags::Default) || vp->hasFlags(ValueProviderFlags::Exclusive);
    if (isDefaultVP)
    {
        // Exclusive value providers replace default value providers.
        if (m_defaultValueProvider)
        {
            ScopeRelock relock(*p);
            m_defaultValueProvider->detach();
            m_defaultValueProvider.reset();
        }
    }

    if (!m_valueProviders)
    {
        m_valueProviders = vp;
        m_defaultValueProvider = vp;
    }
    else
    {
        // Insert the value provider right after the active one.
        vp->prev = m_valueProviders->prev;
        vp->next = m_valueProviders;
        if (vp->prev)
        {
            vp->prev->next = vp;
        }
        m_valueProviders->prev = vp;

        if (!m_defaultValueProvider && isDefaultVP)
        {
            m_defaultValueProvider = vp;
        }
    }
}

PropertyValueProviderSharedPtr PropertyPrivate::removeValueProvider(PropertyValueProviderSharedPtr vp)
{
    P_PTR(Property);
    bool wasEnabled = vp->isEnabled();
    vp->setEnabled(false);

    lock_guard lock(*p);

    if (m_valueProviders == vp)
    {
        m_valueProviders = vp->prev;
        if (m_valueProviders)
        {
            m_valueProviders->next.reset();
        }
    }
    else
    {
        if (vp->prev)
        {
            vp->prev->next = vp->next;
        }
        if (vp->next)
        {
            vp->next->prev = vp->prev;
        }
    }
    vp->prev.reset();
    vp->next.reset();

    if (wasEnabled && m_valueProviders)
    {
        return m_valueProviders;
    }
    return nullptr;
}

void PropertyPrivate::activateValueProvider(PropertyValueProviderSharedPtr vp)
{
    if (m_valueProviders == vp && vp->isEnabled())
    {
        return;
    }

    if (!vp)
    {
        m_valueProviders->setEnabled(true);
        return;
    }

    // remove the vp from the list
    if (vp->prev)
    {
        vp->prev->next = vp->next;
    }
    if (vp->next)
    {
        vp->next->prev = vp->prev;
    }
    vp->next.reset();
    vp->prev.reset();

    // disable head
    m_valueProviders->setEnabled(false);

    // put the vp as head
    vp->prev = m_valueProviders;
    m_valueProviders->next = vp;
    m_valueProviders = vp;
}

void PropertyPrivate::update(const Variant &value)
{
    {
        lock_guard lock(*p_func());
        if (value == m_data.getData())
        {
            return;
        }
        m_data.setData(value);
    }
    p_func()->changed.activate(Callable::ArgumentPack(value));
}



Property::Property(Instance host, PropertyType& type, AbstractPropertyData& data)
    : SharedLock<ObjectLock>(*host.as<ObjectLock>())
    , d_ptr(pimpl::make_d_ptr<PropertyPrivate>(*this, data, type, host))
    , changed(host, type.getChangedSignalType())
{
    d_func()->m_type->addPropertyInstance(*this);
}

Property::~Property()
{
    // Detach the value providers.
    D_PTR(Property);
    // Block property change signal activation.
    changed.setBlocked(true);
    if (d->m_valueProviders)
    {
        d->m_valueProviders->setEnabled(false);
    }
    while (d->m_valueProviders)
    {
        d->m_valueProviders->detach();
    }

    d->m_type->removePropertyInstance(*this);
}

bool Property::isReadOnly() const
{
    return d_func()->m_type->getAccess() == PropertyAccess::ReadOnly;
}

Variant Property::get() const
{
    lock_guard lock(const_cast<Property&>(*this));
    return d_func()->m_data.getData();
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
    d_func()->update(value);
}

void Property::reset()
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    detachValueProviders(ValueProviderFlags::Generic);
    detachValueProviders(ValueProviderFlags::KeepOnWrite);

    auto defaultVp = getDefaultValueProvider();
    throwIf<ExceptionType::MissingPropertyDefaultValueProvider>(!defaultVp);
    set(defaultVp->getLocalValue());
}

PropertyValueProviderSharedPtr Property::getDefaultValueProvider() const
{
    lock_guard lock(const_cast<Property&>(*this));
    return d_func()->m_defaultValueProvider;
}

PropertyValueProviderSharedPtr Property::getExclusiveValueProvider() const
{
    lock_guard lock(const_cast<Property&>(*this));
    D_PTR(Property);

    return (d->m_defaultValueProvider && d->m_defaultValueProvider->hasFlags(ValueProviderFlags::Exclusive))
            ? d->m_defaultValueProvider
            : nullptr;
}

void Property::detachValueProviders(ValueProviderFlags flags)
{
    D_PTR(Property);
    SignalBlocker blockChange(changed);
    for (auto pl = d->m_valueProviders; pl;)
    {
        if (pl->getFlags() == flags)
        {
            auto vp = pl;
            pl = pl->prev;
            vp->detach();
        }
        else
        {
            pl = pl->prev;
        }
    }
}

}
