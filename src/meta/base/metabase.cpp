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

#include <metabase_p.hpp>
#include <signal_p.hpp>
#include <mox/meta/property/property.hpp>

namespace mox
{

void MetaBasePrivate::invalidateDynamicProperties()
{
    auto invalidate = [](auto property)
    {
        auto storage = PropertyStorage::get(*property);
        storage->destroy();
    };
    std::for_each(dynamicProperties.begin(), dynamicProperties.end(), invalidate);
}

void MetaBasePrivate::addSignal(SignalStorage& storage)
{
    signals.insert(std::make_pair(&storage.getType(), &storage));
}

void MetaBasePrivate::removeSignal(SignalStorage* storage)
{
    if (!storage)
    {
        return;
    }
    signals.erase(&storage->getType());
}

void MetaBasePrivate::addProperty(PropertyStorage& storage)
{
    properties.insert(std::make_pair(&storage.getType(), &storage));
}

void MetaBasePrivate::addDynamicProperty(DynamicPropertyPtr property)
{
    dynamicProperties.push_back(property);
}

void MetaBasePrivate::removePropertyStorage(PropertyStorage* storage)
{
    if (!storage)
    {
        return;
    }
    properties.erase(&storage->getType());
}


/******************************************************************************
 * MetaBase
 */
MetaBase::MetaBase()
    : AtomicRefCounted<int32_t>(0)
    , d_ptr(pimpl::make_d_ptr<MetaBasePrivate>(*this))
{
}

#ifdef DEBUG

MetaBase::~MetaBase()
{
    auto dynamicPropertyCount = d_ptr->dynamicProperties.size();
    // Remove dynamic properties. This shall decrease the reference count to the MetaBase.
    d_ptr->invalidateDynamicProperties();

    // The share count shall be at maximum 2 times the dynamic properties count (1 property and 1 change signal).
    FATAL(m_value == 2 * int(dynamicPropertyCount), "Object lock is still shared " << m_value << " times!");
    FATAL(m_lockCount.load() == 0, "Destroying unlocked object! LockCount is " << m_lockCount.load());
    m_lockCount.store(-1);
}

void MetaBase::lock()
{
    FATAL(m_lockCount >= 0, "Invalid MetaBase");

    if (!try_lock())
    {
        // Is this the same owner?
        if (m_owner == std::this_thread::get_id())
        {
            FATAL(false, "Already locked MetaBase! LockCount is " << m_lockCount);
        }
        m_mutex.lock();
        m_owner = std::this_thread::get_id();
        m_lockCount++;
    }
}

void MetaBase::unlock()
{
    FATAL(m_lockCount > 0, "Cannot unlock MetaBase if not locked! LockCount is " << m_lockCount);
    m_mutex.unlock();
    m_lockCount--;
    m_owner = std::thread::id();
}

bool MetaBase::try_lock()
{
    auto result = m_mutex.try_lock();
    if (result)
    {
        m_lockCount++;
        m_owner = std::this_thread::get_id();
    }
    return result;
}

#else

MetaBase::~MetaBase()
{
    // Remove dynamic properties. This shall decrease the reference count to the MetaBase.
    d_ptr->invalidateDynamicProperties();
}

void MetaBase::lock()
{
    m_mutex.lock();
}

void MetaBase::unlock()
{
    m_mutex.unlock();
}

bool MetaBase::try_lock()
{
    return m_mutex.try_lock();
}

#endif

Signal* MetaBase::findSignal(const SignalType& type) const
{
    D();
    auto it = d->signals.find(&type);
    return (it != d->signals.end())
            ? it->second->getSignal()
            : nullptr;
}

Signal* MetaBase::addSignal(const SignalType& type)
{
    UNUSED(type);
    return nullptr;
}

int MetaBase::activateSignal(const SignalType& type, const Callable::ArgumentPack& args)
{
    auto signal = findSignal(type);
    if (!signal)
    {
        return -1;
    }
    return signal->activate(args);
}

Property* MetaBase::findProperty(const PropertyType &type) const
{
    D();
    auto it = d->properties.find(&type);
    return (it != d->properties.end())
            ? it->second->getProperty()
            : nullptr;
}

Property* MetaBase::setProperty(const PropertyType& type, const Variant& value)
{
    auto property = findProperty(type);
    if (property)
    {
        property->set(value);
        return property;
    }

    auto dynamic = DynamicProperty::create(*this, type);
    dynamic->set(value);
    return dynamic.get();
}

Variant MetaBase::getProperty(const PropertyType& type) const
{
    auto property = findProperty(type);
    return (property)
            ? property->get()
            : Variant();
}

} // mox
