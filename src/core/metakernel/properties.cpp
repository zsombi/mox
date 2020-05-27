// Copyright (C) 2020 bitWelder

#include <mox/core/metakernel/properties.hpp>
#include <private/property_p.hpp>

namespace mox { namespace metakernel {

static BindingPtr s_currentBinding;
/******************************************************************************
 * PropertyCorePrivate
 */
PropertyCorePrivate::~PropertyCorePrivate()
{
}

void PropertyCorePrivate::addBinding(BindingCore& binding)
{
    if (activeBinding)
    {
        activeBinding->setEnabled(false);
    }
    activeBinding = binding.shared_from_this();
    bindings.push_back(activeBinding);
    activeBinding->setEnabled(true);
}

void PropertyCorePrivate::removeBinding(BindingCore& binding)
{
    auto shared = binding.shared_from_this();
    auto it = find(bindings, shared);
    if (it)
    {
        it.value().reset();
        if (shared == activeBinding)
        {
            shared->setEnabled(false);
            auto getLast = [](auto& b)
            {
                return b && b->isAttached();
            };
            it = reverse_find_if(bindings, getLast);
            if (it)
            {
                activeBinding = it.value();
                activeBinding->setEnabled(true);
            }
            else
            {
                activeBinding.reset();
            }
        }
    }
}


/******************************************************************************
 * PropertyCore
 */
PropertyCore::PropertyCore()
    : d_ptr(pimpl::make_d_ptr<PropertyCorePrivate>(this))
{
}

PropertyCore::~PropertyCore()
{
    lock_guard lock(d_ptr->bindings);
    auto bindingDetach = [](auto& binding)
    {
        if (binding && binding->isAttached())
        {
            binding->detachFromTarget();
        }
    };
    for_each(d_ptr->bindings, bindingDetach);
}

void PropertyCore::notifyGet(PropertyChangeConnector connectChange) const
{
    auto currentBinding = BindingScope::getCurrent();
    if (currentBinding)
    {
        currentBinding->notifyPropertyAccessed(connectChange);
    }
}

void PropertyCore::notifySet()
{
    D();
    if (BindingScope::getCurrent() && d->activeBinding)
    {
        // Check only the active scope
        if ((d->activeBinding != BindingScope::getCurrent()) && (d->activeBinding->getPolicy() == BindingPolicy::DetachOnWrite))
        {
            d->activeBinding->detachFromTarget();
        }
    }
    else
    {
        // The setter is called because of a simple value assignment
        lock_guard lock(d->bindings);
        auto dropBindings = [](auto& binding)
        {
            if (binding && (binding != BindingScope::getCurrent()) && (binding->getPolicy() == BindingPolicy::DetachOnWrite))
            {
                binding->detachFromTarget();
            }
        };
        for_each(d->bindings, dropBindings);
    }
}

/******************************************************************************
 * BindingCore
 */
BindingCore::BindingCore()
    : d_ptr(pimpl::make_d_ptr<BindingCorePrivate>(this))
{
}

BindingCore::~BindingCore()
{
}

void BindingCore::evaluate()
{
    D();
    if (d->activationCount > 0)
    {
        WARN("binding loop detected!");
        return;
    }
    BindingScope scopeCurrent(*this);
    RefCounter callRef(d->activationCount);
    evaluateOverride();
}

bool BindingCore::isEnabled() const
{
    return d_func()->isEnabled;
}

void BindingCore::setEnabled(bool enabled)
{
    D();
    bool changed = (d->isEnabled != enabled);
    d->isEnabled = enabled;
    if (changed)
    {
        setEnabledOverride();
        if (d->group)
        {
            d->group->setEnabled(d->isEnabled);
        }
    }
}

BindingPolicy BindingCore::getPolicy() const
{
    return d_func()->policy;
}

void BindingCore::setPolicy(BindingPolicy policy)
{
    d_func()->policy = policy;
    setPolicyOverride();
}

void BindingCore::setGroup(BindingGroupPtr group)
{
    d_func()->group = group;
}

bool BindingCore::isAttached() const
{
    D();
    return d->status == BindingCorePrivate::Status::Attaching || d->status == BindingCorePrivate::Status::Attached;
}

void BindingCore::attachToTarget(PropertyCore& property)
{
    D();
    throwIf<ExceptionType::BindingAttached>(d->status == BindingCorePrivate::Status::Attached);
    if (d->status == BindingCorePrivate::Status::Attaching)
    {
        return;
    }
    d->status = BindingCorePrivate::Status::Attaching;
    d->target = &property;
    PropertyCorePrivate::get(*d->target)->addBinding(*this);
    d->status = BindingCorePrivate::Status::Attached;
}

void BindingCore::detachFromTarget()
{
    D();
    throwIf<ExceptionType::BindingDetached>(d->status == BindingCorePrivate::Status::Detached);
    if (d->status == BindingCorePrivate::Status::Detaching)
    {
        return;
    }
    auto d_target = PropertyCorePrivate::get(*d->target);
    lock_guard lock(d_target->bindings);
    auto keepAlive = shared_from_this();
    d->status = BindingCorePrivate::Status::Detaching;
    d_target->removeBinding(*this);
    if (d->group)
    {
        auto grp = d->group;
        d->group->removeFromGroup(*this);
        grp->discard();
    }
    detachOverride();
    d->target = nullptr;
    d->status = BindingCorePrivate::Status::Detached;
}

/******************************************************************************
 * BindingScope
 */
BindingScope::BindingScope(BindingCore& currentBinding)
    : m_previousBinding(s_currentBinding)
{
    s_currentBinding = currentBinding.shared_from_this();
}

BindingScope::~BindingScope()
{
    s_currentBinding = m_previousBinding.lock();
}

BindingPtr BindingScope::getCurrent()
{
    return s_currentBinding;
}

/******************************************************************************
 * BindingGroupd
 */
BindingGroup::BindingGroup()
{
}

BindingGroup::~BindingGroup()
{
    auto looper = [](auto& binding)
    {
        if (binding)
        {
            binding->setGroup(nullptr);
        }
    };
    for_each(m_bindings, looper);
}

BindingGroupPtr BindingGroup::create()
{
    return BindingGroupPtr(new BindingGroup);
}

void BindingGroup::discard()
{
    auto detacher = [](auto& binding)
    {
        if (binding)
        {
            binding->detachFromTarget();
        }
    };
    for_each(m_bindings, detacher);
}

BindingGroup& BindingGroup::addToGroup(BindingCore& binding)
{
    m_bindings.push_back(binding.shared_from_this());
    binding.setGroup(as_shared<BindingGroup>(this));
    binding.setPolicy(m_policy);
    binding.setEnabled(m_isEnabled);
    return *this;
}

void BindingGroup::removeFromGroup(BindingCore& binding)
{
    auto keepAlive = shared_from_this();
    erase(m_bindings, binding.shared_from_this());
    binding.setGroup(nullptr);
}

void BindingGroup::setPolicy(BindingPolicy policy)
{
    m_policy = policy;
    auto update = [this](auto& binding)
    {
        if (binding)
        {
            binding->setPolicy(m_policy);
        }
    };
    for_each(m_bindings, update);
}

bool BindingGroup::isEnabled() const
{
    return m_isEnabled;
}

void BindingGroup::setEnabled(bool enabled)
{
    if (m_isUpdating)
    {
        return;
    }
    ScopeValue lock(m_isUpdating, true);
    m_isEnabled = enabled;

    auto update = [this](auto& binding)
    {
        if (binding)
        {
            binding->setEnabled(m_isEnabled);
        }
    };
    for_each(m_bindings, update);
}

}} // mox::metakernel
