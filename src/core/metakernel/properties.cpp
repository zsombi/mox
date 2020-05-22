// Copyright (C) 2020 bitWelder

#include <mox/core/metakernel/properties.hpp>
#include <private/property_p.hpp>

namespace mox { namespace metakernel {

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
    m_bindings.push_back(binding.shared_from_this());
}

void PropertyCore::removeBinding(BindingCore& binding)
{
    erase(m_bindings, binding.shared_from_this());
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
    }
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
        m_group->removeFromGroup(*this);
    }
    detachOverride();
    m_target = nullptr;
    m_status = Status::Detached;
}

/******************************************************************************
 * BindingGroupd
 */
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
    m_bindings.clear();
}

void BindingGroup::addToGroup(BindingCore& binding)
{
    m_bindings.push_back(binding.shared_from_this());
    binding.setGroup(as_shared<BindingGroup>(this));
}

void BindingGroup::removeFromGroup(BindingCore& binding)
{
    auto keepAlive = shared_from_this();
    binding.setGroup(nullptr);
    erase(m_bindings, binding.shared_from_this());
}

void BindingGroup::setEnabledOverride()
{
    auto update = [this](auto& binding)
    {
        if (binding)
        {
            binding->setEnabled(this->isEnabled());
        }
    };
    for_each(m_bindings, update);
}

}} // mox::metakernel
