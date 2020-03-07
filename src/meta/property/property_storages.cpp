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

#include <property_p.hpp>
#include <signal_p.hpp>
#include <binding_p.hpp>
#include <metabase_p.hpp>

namespace mox
{

/******************************************************************************
 * PropertyStorage
 */
PropertyStorage::PropertyStorage(Property& property, MetaBase& host, const PropertyType& type, PropertyDataProvider& dataProvider)
    : p_ptr(&property)
    , type(type)
    , host(host)
    , dataProvider(dataProvider)
{
    MetaBasePrivate::get(host)->addProperty(*this);
    this->dataProvider.m_property = this;
}

PropertyStorage::~PropertyStorage()
{
}

void PropertyStorage::destroy()
{
    clearBindings();

    lock_guard lock(host);
    // Clear subscribers
    while (!bindingSubscribers.empty())
    {
        auto subscriber = *bindingSubscribers.begin();
        auto pSubscriber = BindingPrivate::get(*subscriber);
        // The property is dying, so the binding subscribed to it shall too.
        if (subscriber->isAttached())
        {
            ScopeRelock relock(host);
            subscriber->detach();
        }
        else
        {
            erase(bindings, subscriber);
            {
                ScopeRelock relock(host);
                pSubscriber->clearDependencies();
            }
        }
        pSubscriber->invalidate();
    }
    bindingSubscribers.clear();

    dataProvider.m_property = nullptr;
    // Destroy the storage of the change signal.
    SignalStorage::get(p_ptr->changed)->destroy();
    // Self destroy.
    p_ptr->d_ptr.reset();
}

BindingSharedPtr PropertyStorage::getTopBinding()
{
    return !bindings.empty() && bindings.back()->isEnabled() ? bindings.back() : nullptr;
}

void PropertyStorage::resetToDefault()
{
    // Detach all bindings, and restore the default value.
    clearBindings();
    updateData(type.getDefault());
}

void PropertyStorage::notifyAccessed()
{
    if (BindingScope::currentBinding && BindingScope::currentBinding->getTarget() != p_ptr)
    {
        bindingSubscribers.insert(BindingScope::currentBinding->shared_from_this());

        auto currentBinding = BindingPrivate::get(*BindingScope::currentBinding);
        currentBinding->addDependency(*p_ptr);
    }
}

void PropertyStorage::notifyChanges()
{
    auto copy = SubscriberCollection();
    {
        lock_guard lock(host);
        copy = bindingSubscribers;
    }
    for (auto subscriber : copy)
    {
        if (!subscriber->isEnabled())
        {
            continue;
        }
        subscriber->evaluateBinding();
    }
}

void PropertyStorage::unsubscribe(BindingSharedPtr binding)
{
    lock_guard lock(host);
    bindingSubscribers.erase(binding);
}

Variant PropertyStorage::fetchDataUnsafe() const
{
    return dataProvider.getData();
}

void PropertyStorage::clearBindings()
{
    // Block property change signal activation.
    SignalBlocker block(p_ptr->changed);
    lock_guard lock(host);
    while (!bindings.empty())
    {
        ScopeRelock relock(host);
        bindings.front()->detach();
    }
}

void PropertyStorage::addBinding(BindingSharedPtr binding)
{
    lock_guard lock(host);
    if (!bindings.empty())
    {
        bindings.back()->setEnabled(false);
    }

    bindings.push_back(binding);
}

void PropertyStorage::removeBinding(Binding& binding)
{
    lock_guard lock(host);
    erase(bindings, binding.shared_from_this());
}

void PropertyStorage::detachNonPermanentBindings()
{
    // lock as we mangle property data
    lock_guard lock(host);
    SignalBlocker block(p_ptr->changed);

    auto copy = bindings;
    for (auto it = copy.begin(); it != copy.end(); ++it)
    {
        auto binding = *it;
        if (binding->isPermanent())
        {
            continue;
        }

        ScopeRelock relock(host);
        binding->detach();
    }

    // Mark the top binding as enabled, silently.
    if (!bindings.empty() && bindings.back()->isAttached())
    {
        BindingPrivate::get(*bindings.back())->setEnabled(true);
    }
}

void PropertyStorage::tryActivateHeadBinding()
{
    auto binding = BindingSharedPtr();
    {
        lock_guard lock(host);
        if (bindings.empty())
        {
            return;
        }
        binding = bindings.back();
    }
    binding->setEnabled(true);
}

// Moves the binding to the front.
void PropertyStorage::activateBinding(Binding& binding)
{
    lock_guard lock(host);
    if (!bindings.empty() && bindings.back().get() == &binding)
    {
        return;
    }
    bindings.back()->setEnabled(false);

    auto shBinding = binding.shared_from_this();
    erase(bindings, shBinding);
    bindings.push_back(shBinding);
}

void PropertyStorage::updateData(const Variant& newValue)
{
    {
        lock_guard lock(host);
        if (newValue == dataProvider.getData())
        {
            return;
        }
        dataProvider.setData(newValue);
    }

    notifyChanges();

    p_ptr->changed.activate(Callable::ArgumentPack(newValue));
}

}
