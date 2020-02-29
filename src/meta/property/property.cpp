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

#include <signal_p.hpp>
#include <property_p.hpp>
#include <mox/meta/property/property.hpp>
#include <mox/config/error.hpp>
#include <mox/binding/binding.hpp>

#include <binding_p.hpp>

namespace mox
{

void AbstractPropertyData::updateData(const Variant& newValue)
{
    m_property->updateData(newValue);
}


PropertyPrivate::PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, ObjectLock& host)
    : p_ptr(&p)
    , dataProvider(data)
    , type(&type)
    , host(&host)
{
    dataProvider.m_property = this;
}

PropertyPrivate::~PropertyPrivate()
{
    TRACE("Private data for property discarded");
}

void PropertyPrivate::resetToDefault()
{
    // Detach all bindings, and restore the default value.
    clearBindings();
    updateData(type->getDefault());
}

void PropertyPrivate::notifyAccessed()
{
    P();
    if (BindingScope::currentBinding && BindingScope::currentBinding->getTarget() != p)
    {
        bindingSubscribers.insert(BindingScope::currentBinding->shared_from_this());

        auto currentBinding = BindingPrivate::get(*BindingScope::currentBinding);
        currentBinding->addDependency(*p);
    }
}

void PropertyPrivate::notifyChanges()
{
    auto copy = SubscriberCollection();
    {
        lock_guard lock(*p_func());
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

void PropertyPrivate::unsubscribe(BindingSharedPtr binding)
{
    lock_guard lock(*p_func());
    bindingSubscribers.erase(binding);
}

Variant PropertyPrivate::fetchDataUnsafe() const
{
    return dataProvider.getData();
}

void PropertyPrivate::clearBindings()
{
    P();

    // Block property change signal activation.
    SignalBlocker block(p->changed);
    lock_guard lock(*p);
    while (!bindings.empty())
    {
        ScopeRelock relock(*p);
        bindings.front()->detach();
    }
}

void PropertyPrivate::addBinding(BindingSharedPtr binding)
{
    lock_guard lock(*p_func());
    if (!bindings.empty())
    {
        bindings.back()->setEnabled(false);
    }

    bindings.push_back(binding);
}

void PropertyPrivate::removeBinding(Binding& binding)
{
    lock_guard lock(*p_func());
    erase(bindings, binding.shared_from_this());
}

void PropertyPrivate::tryActivateHeadBinding()
{
    auto binding = BindingSharedPtr();
    {
        lock_guard lock(*p_func());
        if (bindings.empty())
        {
            return;
        }
        binding = bindings.back();
    }
    binding->setEnabled(true);
}

// Moves the binding to the front.
void PropertyPrivate::activateBinding(Binding& binding)
{
    lock_guard lock(*p_func());
    if (!bindings.empty() && bindings.back().get() == &binding)
    {
        return;
    }
    bindings.back()->setEnabled(false);

    auto shBinding = binding.shared_from_this();
    erase(bindings, shBinding);
    bindings.push_back(shBinding);
}

void PropertyPrivate::updateData(const Variant& newValue)
{
    P();
    {
        lock_guard lock(*p);
        if (newValue == dataProvider.getData())
        {
            return;
        }
        dataProvider.setData(newValue);
    }

    notifyChanges();

    p->changed.activate(Callable::ArgumentPack(newValue));
}

/******************************************************************************
 * Property - public API
 */

Property::Property(ObjectLock& host, PropertyType& type, AbstractPropertyData& data)
    : SharedLock(host)
    , d_ptr(pimpl::make_d_ptr<PropertyPrivate>(*this, data, type, host))
    , changed(host, type.getChangedSignalType())
{
    D();
    d->type->addPropertyInstance(*this);
    data.initialize();
}

Property::~Property()
{
    D();

    lock_guard lock(*this);
    // First make it invalid. Remove the instance from the property type, and reset the host.
    d->type->removePropertyInstance(*this);
    d->type = nullptr;
    d->host = nullptr;

    {
        ScopeRelock relock(*this);
        d->clearBindings();
    }

    // Clear subscribers
    while (!d->bindingSubscribers.empty())
    {
        auto subscriber = *d->bindingSubscribers.begin();
        auto pSubscriber = BindingPrivate::get(*subscriber);
        // The property is dying, so the binding subscribed to it shall too.
        if (subscriber->isAttached())
        {
            ScopeRelock relock(*this);
            subscriber->detach();
        }
        else
        {
            erase(d->bindings, subscriber);
            {
                ScopeRelock relock(*this);
                pSubscriber->clearDependencies();
            }
        }
        pSubscriber->invalidate();
    }
    d->bindingSubscribers.clear();
    TRACE("Property died");
}

bool Property::isValid() const
{
    return d_ptr && d_func()->type != nullptr;
}

bool Property::isReadOnly() const
{
    return d_ptr && (d_func()->type->getAccess() == PropertyAccess::ReadOnly);
}

Variant Property::get() const
{
    FATAL(isValid(), "Invalid property accessed")
    lock_guard lock(const_cast<Property&>(*this));
    D();
    const_cast<PropertyPrivate*>(d)->notifyAccessed();
    return d->fetchDataUnsafe();
}

void Property::set(const Variant& value)
{
    FATAL(isValid(), "Invalid property accessed")
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    D();

    // Detach bindings that are not permanent.
    {
        // lock as we mangle property data
        lock_guard lock(*this);
        SignalBlocker block(changed);

        auto copy = d->bindings;
        for (auto it = copy.begin(); it != copy.end(); ++it)
        {
            auto binding = *it;
            if (binding->isPermanent())
            {
                continue;
            }

            ScopeRelock relock(*this);
            binding->detach();
        }

        // Mark the top binding as enabled, silently.
        if (!d->bindings.empty() && d->bindings.back()->isAttached())
        {
            BindingPrivate::get(*d->bindings.back())->setEnabled(true);
        }
    }

    // Set the value.
    d->updateData(value);
}

void Property::reset()
{
    FATAL(isValid(), "Invalid property accessed")
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    d_func()->resetToDefault();
}

BindingSharedPtr Property::getCurrentBinding()
{
    FATAL(isValid(), "Invalid property accessed")
    D();
    lock_guard lock(*this);
    return !d->bindings.empty() && d->bindings.back()->isEnabled() ? d->bindings.back() : nullptr;
}

}
