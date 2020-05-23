// Copyright (C) 2020 bitWelder

#include <mox/core/metakernel/properties.hpp>
#include <private/property_p.hpp>

namespace mox { namespace metakernel {

static BindingPtr s_currentBinding;
/******************************************************************************
 * PropertyCorePrivate
 */
/******************************************************************************
 * PropertyCore::Data
 */
PropertyCore::Data::Data(PropertyType type)
    : propertyType(type)
{
}

/******************************************************************************
 * PropertyCore
 */
PropertyCore::PropertyCore(Data& data, SignalCore& changedSignal)
    : m_data(data)
    , m_changedSignal(changedSignal)
{
}

PropertyCore::~PropertyCore()
{
    lock_guard lock(m_bindings);
    auto bindingDetach = [](auto& binding)
    {
        if (binding && binding->isAttached())
        {
            binding->detachFromTarget();
        }
    };
    for_each(m_bindings, bindingDetach);
}

PropertyType PropertyCore::getType() const
{    return m_data.propertyType;
}

PropertyCore::Data& PropertyCore::getDataProvider()
{
    return m_data;
}

PropertyCore::Data& PropertyCore::getDataProvider() const
{
    return m_data;
}

void PropertyCore::addBinding(BindingCore& binding)
{
    if (m_activeBinding)
    {
        m_activeBinding->setEnabled(false);
    }
    m_activeBinding = binding.shared_from_this();
    m_bindings.push_back(m_activeBinding);
    m_activeBinding->setEnabled(true);
}

void PropertyCore::removeBinding(BindingCore& binding)
{
    auto shared = binding.shared_from_this();
    auto it = find(m_bindings, shared);
    if (it)
    {
        it.value().reset();
        if (shared == m_activeBinding)
        {
            shared->setEnabled(false);
            auto getLast = [](auto& b)
            {
                return b && b->isAttached();
            };
            it = reverse_find_if(m_bindings, getLast);
            if (it)
            {
                m_activeBinding = it.value();
                m_activeBinding->setEnabled(true);
            }
            else
            {
                m_activeBinding.reset();
            }
        }
    }
}

void PropertyCore::notifyGet() const
{

}

void PropertyCore::notifySet()
{
    if (BindingScope::getCurrent() && m_activeBinding)
    {
        // Check only the active scope
        if ((m_activeBinding != BindingScope::getCurrent()) && (m_activeBinding->getPolicy() == BindingPolicy::DetachOnWrite))
        {
            m_activeBinding->detachFromTarget();
        }
    }
    else
    {
        // The setter is called because of a simple value assignment
        lock_guard lock(m_bindings);
        auto dropBindings = [](auto& binding)
        {
            if (binding && (binding != BindingScope::getCurrent()) && (binding->getPolicy() == BindingPolicy::DetachOnWrite))
            {
                binding->detachFromTarget();
            }
        };
        for_each(m_bindings, dropBindings);
    }
}

ArgumentData PropertyCore::get() const
{
    return m_data.get();
}

void PropertyCore::set(const ArgumentData& data)
{
    if (!m_data.isEqual(data))
    {
        m_data.set(data);
        auto signalArg = PackedArguments();
        signalArg += data;
        m_changedSignal.activate(signalArg);
    }
}

/******************************************************************************
 * BindingCore
 */
BindingCore::BindingCore()
{
}

BindingCore::~BindingCore()
{
}

void BindingCore::evaluate()
{
    BindingScope scopeCurrent(*this);
    evaluateOverride();
}

bool BindingCore::isEnabled() const
{
    return m_isEnabled;
}

void BindingCore::setEnabled(bool enabled)
{
    bool changed = (m_isEnabled != enabled);
    m_isEnabled = enabled;
    if (changed)
    {
        setEnabledOverride();
        if (m_group)
        {
            m_group->setEnabled(m_isEnabled);
        }
    }
}

BindingPolicy BindingCore::getPolicy() const
{
    return m_policy;
}

void BindingCore::setPolicy(BindingPolicy policy)
{
    m_policy = policy;
    setPolicyOverride();
}

void BindingCore::setGroup(BindingGroupPtr group)
{
    m_group = group;
}

bool BindingCore::isAttached() const
{
    return m_status == Status::Attaching || m_status == Status::Attached;
}

void BindingCore::attachToTarget(PropertyCore& property)
{
    throwIf<ExceptionType::BindingAttached>(m_status == Status::Attached);
    if (m_status == Status::Attaching)
    {
        return;
    }
    m_status = Status::Attaching;
    m_target = &property;
    m_target->addBinding(*this);
    m_status = Status::Attached;
}

void BindingCore::detachFromTarget()
{
    throwIf<ExceptionType::BindingDetached>(m_status == Status::Detached);
    if (m_status == Status::Detaching)
    {
        return;
    }
    lock_guard lock(m_target->m_bindings);
    auto keepAlive = shared_from_this();
    m_status = Status::Detaching;
    m_target->removeBinding(*this);
    if (m_group)
    {
        auto grp = m_group;
        m_group->removeFromGroup(*this);
        grp->discard();
    }
    detachOverride();
    m_target = nullptr;
    m_status = Status::Detached;
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
