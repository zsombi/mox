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
#include <mox/binding/binding.hpp>

#include <binding_p.hpp>

namespace mox
{

void AbstractPropertyData::accessed() const
{
    auto pp = PropertyPrivate::get(*m_property);
    const_cast<PropertyPrivate*>(pp)->notifyAccessed();
}

void AbstractPropertyData::updateData(const Variant& newValue)
{
    if (newValue == getData())
    {
        return;
    }
    else
    {
        // Use this syntax to benefit the scope for data setting time only
        lock_guard lock(*m_property);
        setData(newValue);
    }

    auto pp = PropertyPrivate::get(*m_property);
    pp->notifyChanges();

    m_property->changed.activate(Callable::ArgumentPack(newValue));
}


PropertyPrivate::PropertyPrivate(Property& p, AbstractPropertyData& data, PropertyType& type, Instance host)
    : p_ptr(&p)
    , dataProvider(data)
    , type(&type)
    , host(host)
{
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
    auto copy = bindingSubscribers;
    for (auto subscriber : copy)
    {
        if (!subscriber->isEnabled())
        {
            continue;
        }
        auto dSubscriber = BindingPrivate::get(*subscriber);
        dSubscriber->evaluateBinding();
    }
}

void PropertyPrivate::clearAllSubscribers()
{
    while (!bindingSubscribers.empty())
    {
        auto subscriber = *bindingSubscribers.begin();
        auto pSubscriber = BindingPrivate::get(*subscriber);
        // The property is dying, so the binding subscribed to it shall too.
        if (subscriber->isAttached())
        {
            subscriber->detach();
        }
        else
        {
            eraseBinding(*subscriber);
            pSubscriber->clearDependencies();
        }
        auto pBinding = BindingPrivate::get(*subscriber);
        pBinding->state = BindingState::Invalid;
    }
    bindingSubscribers.clear();
}

void PropertyPrivate::clearBindings()
{
    P();
    // Block property change signal activation.
    SignalBlocker block(p->changed);

    while (bindingsHead)
    {
        auto keepAlive = bindingsHead;
        auto pBinding = BindingPrivate::get(*bindingsHead);

        eraseBinding(*bindingsHead);
        pBinding->detachFromTarget();
    }
}

void PropertyPrivate::removeDetachableBindings()
{
    P();
    SignalBlocker block(p->changed);

    for (auto pl = bindingsHead; pl;)
    {
        if (pl->isPermanent())
        {
            pl = BindingPrivate::get(*pl)->prev;
            continue;
        }

        auto pd = pl;
        // advance pl
        pl = BindingPrivate::get(*pl)->prev;

        auto ppd = BindingPrivate::get(*pd);

        // Detach from the binding list.
        eraseBinding(*pd);
        ppd->detachFromTarget();
    }

    // Mark the top binding as enabled, silently.
    if (bindingsHead && bindingsHead->isAttached())
    {
        BindingPrivate::get(*bindingsHead)->enabled = true;
    }
}

void PropertyPrivate::eraseBinding(Binding &binding)
{
    auto pBinding = BindingPrivate::get(binding);
    if (bindingsHead.get() == &binding)
    {
        bindingsHead = pBinding->prev;
        if (bindingsHead)
        {
            BindingPrivate::get(*bindingsHead)->next.reset();
        }
    }
    else
    {
        if (pBinding->prev)
        {
            BindingPrivate::get(*pBinding->prev)->next = pBinding->next;
        }
        if (pBinding->next)
        {
            BindingPrivate::get(*pBinding->next)->prev = pBinding->prev;
        }
    }
    pBinding->prev.reset();
    pBinding->next.reset();
}

void PropertyPrivate::addBinding(BindingSharedPtr binding)
{
    BindingPrivate::get(*binding)->prev = bindingsHead;
    if (bindingsHead)
    {
        BindingPrivate::get(*bindingsHead)->next = binding;
    }
    bindingsHead = binding;
}


Property::Property(Instance host, PropertyType& type, AbstractPropertyData& data)
    : SharedLock<ObjectLock>(*host.as<ObjectLock>())
    , d_ptr(pimpl::make_d_ptr<PropertyPrivate>(*this, data, type, host))
    , changed(host, type.getChangedSignalType())
{
    D();
    d->dataProvider.m_property = this;
    d->type->addPropertyInstance(*this);
}

Property::~Property()
{
    D();
    d->clearBindings();
    d->clearAllSubscribers();
    d->type->removePropertyInstance(*this);
}

AbstractPropertyData* Property::getDataProvider() const
{
    return &d_func()->dataProvider;
}

bool Property::isReadOnly() const
{
    return d_func()->type->getAccess() == PropertyAccess::ReadOnly;
}

Variant Property::get() const
{
    lock_guard lock(const_cast<Property&>(*this));
    return d_func()->dataProvider.getData();
}

void Property::set(const Variant& value)
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    D();
    // Detach bindings that are not permanent.
    d->removeDetachableBindings();

    // Set the value.
    d->dataProvider.updateData(value);
}

void Property::reset()
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());

    D();
    // Detach all bindings, and restore the default value.
    d->clearBindings();
    d->dataProvider.resetToDefault();
}

void Property::addBinding(BindingSharedPtr binding)
{
    throwIf<ExceptionType::AttemptAttachingBindingToReadOnlyProperty>(isReadOnly());
    throwIf<ExceptionType::InvalidArgument>(!binding);
    throwIf<ExceptionType::InvalidBinding>(!binding->isValid());
    if (binding->getState() == BindingState::Attaching)
    {
        return;
    }

    throwIf<ExceptionType::BindingAlreadyAttached>(binding->isAttached());

    D();
    if (d->bindingsHead)
    {
        d->bindingsHead->setEnabled(false);
    }

    d->addBinding(binding);
    auto pBinding = BindingPrivate::get(*binding);
    pBinding->attachToTarget(*this);

    binding->setEnabled(true);

    if (!pBinding->evaluateOnEnabled)
    {
        pBinding->evaluateBinding();
    }
}

void Property::removeBinding(Binding& binding)
{    
    if (binding.getState() == BindingState::Detaching)
    {
        return;
    }

    throwIf<ExceptionType::InvalidArgument>(!binding.isAttached());
    throwIf<ExceptionType::WrongBindingTarget>(binding.getTarget() != this);

    bool wasEnabled = binding.isEnabled();
    auto keepAlive = binding.shared_from_this();

    D();
    d->eraseBinding(binding);
    auto pBinding = BindingPrivate::get(binding);
    pBinding->detachFromTarget();

    if (wasEnabled && d->bindingsHead)
    {
        d->bindingsHead->setEnabled(true);
    }
}

BindingSharedPtr Property::getCurrentBinding()
{
    return d_func()->bindingsHead;
}

// Moves the binding to the front.
void PropertyPrivate::activateBinding(Binding& binding)
{
    throwIf<ExceptionType::InvalidArgument>(!binding.isAttached());

    if (bindingsHead.get() == &binding)
    {
        return;
    }

    bindingsHead->setEnabled(false);
    eraseBinding(binding);
    addBinding(binding.shared_from_this());
}

}
