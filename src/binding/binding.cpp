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

#include <binding_p.hpp>
#include <property_p.hpp>
#include <mox/binding/binding_group.hpp>

namespace mox
{

/******************************************************************************
 * BindingPrivate
 */
BindingPrivate::BindingPrivate(Binding* pp, bool permanent)
    : p_ptr(pp)
    , isPermanent(permanent)
{
}

BindingPrivate::~BindingPrivate()
{
}

void BindingPrivate::attachToTarget(Property& target)
{
    P_PTR(Binding);
    throwIf<ExceptionType::BindingAlreadyAttached>(p->isAttached());

    state = BindingState::Attaching;
    this->target = &target;
    p->onAttached();
    state = BindingState::Attached;
}

void BindingPrivate::detachFromTarget()
{
    P_PTR(Binding);
    throwIf<ExceptionType::BindingNotAttached>(!p->isAttached());

    state = BindingState::Detaching;
    p->onDetached();
    clearDependencies();

    if (group)
    {
        group->detach();
    }

    target = nullptr;
    enabled = false;
    state = BindingState::Detached;
}

void BindingPrivate::addDependency(Property &dependency)
{
    dependencies.insert(&dependency);
}

void BindingPrivate::removeDependency(Property &dependency)
{
    dependencies.erase(&dependency);
}

void BindingPrivate::clearDependencies()
{
    auto psh = p_func()->shared_from_this();
    for (auto dep : dependencies)
    {
        auto ddep = PropertyPrivate::get(*dep);
        ddep->bindingSubscribers.erase(psh);
    }
    dependencies.clear();
}

void BindingPrivate::evaluateBinding()
{
    if (!enabled)
    {
        return;
    }

    clearDependencies();
    PropertyPrivate::Scope setCurrent(*target);
    p_func()->evaluate();
}

/******************************************************************************
 * Binding
 */

Binding::Binding(bool permanent)
    : d_ptr(pimpl::make_d_ptr<BindingPrivate>(this, permanent))
{
}

Binding::Binding(pimpl::d_ptr_type<BindingPrivate> dd)
    : d_ptr(std::move(dd))
{
}

Binding::~Binding()
{
//    std::cout << "Binding died" << std::endl;
}

bool Binding::isAttached() const
{
    return d_func()->target != nullptr;
}

bool Binding::isPermanent() const
{
    return d_func()->isPermanent;
}

bool Binding::isEnabled() const
{
    return d_func()->enabled;
}
void Binding::setEnabled(bool enabled)
{
    D();
    if (d->enabled == enabled)
    {
        return;
    }

    d->enabled = enabled;

    if (d->enabled && d->target)
    {
        PropertyPrivate::get(*d->target)->activateBinding(*this);
    }

    onEnabledChanged();

    if (d->evaluateOnEnabled)
    {
        d->evaluateBinding();
    }
}

bool Binding::doesEvaluateOnEnabled() const
{
    return d_func()->evaluateOnEnabled;
}
void Binding::setEvaluateOnEnabled(bool doEvaluate)
{
    d_func()->evaluateOnEnabled = doEvaluate;
}

Property* Binding::getTarget() const
{
    return d_func()->target;
}

void Binding::detach()
{
    if (!isAttached())
    {
        return;
    }
    d_func()->target->removeBinding(*this);
}

BindingGroupSharedPtr Binding::getBindingGroup()
{
    return d_func()->group;
}

} // mox
