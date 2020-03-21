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
#include <mox/core/meta/property/binding/binding_group.hpp>

namespace mox
{

/******************************************************************************
 * BindingPrivate
 */
BindingPrivate::BindingPrivate(Binding* pp, bool permanent)
    : p_ptr(pp)
    , enabled(false)
    , evaluateOnEnabled(true)
    , isPermanent(permanent)
{
}

BindingPrivate::~BindingPrivate()
{
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
        auto ddep = PropertyStorage::get(*dep);
        FATAL(ddep, "Property storage for the dependency already wiped!");
        ddep->unsubscribe(psh);
    }
    dependencies.clear();
}

void BindingPrivate::invalidate()
{
    state = BindingState::Invalid;
}

/******************************************************************************
 * BindingLoopDetector
 */
BindingLoopDetector::BindingLoopDetector(BindingPrivate& binding)
    : BaseClass(binding)
{
    prev = last;
    last = this;
    if (m_refCounted.group && m_refCounted.group->getNormalizer())
    {
        m_refCounted.group->getNormalizer()->retain();
    }
}
BindingLoopDetector::~BindingLoopDetector()
{
    FATAL(last == this, "Some other binding messed up the binding loop detection");
    last = prev;
    if (m_refCounted.group && m_refCounted.group->getNormalizer())
    {
        m_refCounted.group->getNormalizer()->release();
    }
}
bool BindingLoopDetector::tryNormalize(Variant& value)
{
    auto groupNormalizer = m_refCounted.group ? m_refCounted.group->getNormalizer() : nullptr;
    if (m_refCounted.m_value > 1)
    {
        // Without group and normalizer, throw exception.
        throwIf<ExceptionType::BindingLoop>(!groupNormalizer);
        auto normalizer = m_refCounted.group->getNormalizer();

        switch (normalizer->tryNormalize(*m_refCounted.p_ptr, value, m_refCounted.m_value))
        {
            case BindingNormalizer::Normalized:
            {
                return true;
            }
            case BindingNormalizer::FailAndExit:
            {
                // The normalization failed, exit.
                normalizer->reset();
                return false;
            }
            case BindingNormalizer::Throw:
            {
                normalizer->reset();
                throwIf<ExceptionType::BindingLoop>(true);
            }
        }
    }
    else if (groupNormalizer)
    {
        groupNormalizer->initialize(*m_refCounted.p_ptr, value);
    }
    return true;
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
}

void Binding::attach(Property& target)
{
    throwIf<ExceptionType::InvalidArgument>(!target.isValid());
    throwIf<ExceptionType::AttemptAttachingBindingToReadOnlyProperty>(target.isReadOnly());
    throwIf<ExceptionType::InvalidBinding>(!isValid());

    if (getState() == BindingState::Attaching)
    {
        return;
    }
    throwIf<ExceptionType::BindingAlreadyAttached>(isAttached());

    auto dTarget = PropertyStorage::get(target);
    dTarget->addBinding(shared_from_this());

    D();
    d->target = &target;

    d->state = BindingState::Attaching;
    onAttached();
    d->state = BindingState::Attached;

    setEnabled(true);

    if (!d->evaluateOnEnabled)
    {
        evaluateBinding();
    }
}

void Binding::detach()
{
    if (getState() == BindingState::Detaching)
    {
        return;
    }

    throwIf<ExceptionType::InvalidArgument>(!isAttached());

    bool wasEnabled = isEnabled();
    auto keepAlive = shared_from_this();

    auto dTarget = PropertyStorage::get(*getTarget());
    dTarget->removeBinding(*this);

    // Detach from target.
    D();
    d->state = BindingState::Detaching;
    onDetached();
    d->clearDependencies();

    if (d->group)
    {
        d->group->detach();
    }

    d->target = nullptr;
    d->enabled = false;
    d->state = BindingState::Detached;

    // If this was the enabled one, enable the head binding of the property
    if (wasEnabled)
    {
        dTarget->tryActivateHeadBinding();
    }
}

void Binding::evaluateBinding()
{
    D();
    if (!d->enabled)
    {
        return;
    }

    auto dTarget = PropertyStorage::get(*d->target);
    Variant data = dTarget->fetchDataUnsafe();
    BindingLoopDetector detector(*d);

    d->clearDependencies();
    BindingScope setCurrent(*this);
    evaluate();
}

void Binding::updateTarget(Variant &value)
{
    if (!BindingLoopDetector::getCurrent()->tryNormalize(value))
    {
        return;
    }
    auto dTarget = PropertyStorage::get(*d_func()->target);
    dTarget->updateData(value);
}

bool Binding::isValid() const
{
    return d_func()->state != BindingState::Invalid;
}

bool Binding::isAttached() const
{
    return d_func()->target != nullptr;
}

BindingState Binding::getState() const
{
    return d_func()->state;
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
    if (!isAttached())
    {
        return;
    }

    if (d->enabled == enabled)
    {
        return;
    }

    d->enabled = enabled;

    if (d->enabled && d->target)
    {
        PropertyStorage::get(*d->target)->activateBinding(*this);
    }

    onEnabledChanged();

    if (d->evaluateOnEnabled)
    {
        evaluateBinding();
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

BindingGroupSharedPtr Binding::getBindingGroup()
{
    return d_func()->group;
}

} // mox
