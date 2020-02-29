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
#include <mox/utils/locks.hpp>
#include <metadata_p.hpp>
#include <mox/object.hpp>

#include <mox/module/thread_loop.hpp>

namespace mox
{

/******************************************************************************
 * SignalType
 */
SignalType::SignalType(VariantDescriptorContainer&& args)
    : m_argumentDescriptors(std::forward<VariantDescriptorContainer>(args))
{
}

Signal* SignalType::getSignalForInstance(ObjectLock& instance) const
{
    lock_guard lock(const_cast<SignalType&>(*this));
    auto it = m_instances.find(&instance);
    if (it != m_instances.cend())
    {
        return it->second;
    }
    return nullptr;
}

int SignalType::activate(ObjectLock& sender, const Callable::ArgumentPack &args) const
{
    auto signal = getSignalForInstance(sender);
    if (!signal)
    {
        return -1;
    }
    return signal->activate(args);
}

bool SignalType::isCompatible(const SignalType &other) const
{
    return m_argumentDescriptors.isInvocableWith(other.m_argumentDescriptors);
}

const VariantDescriptorContainer& SignalType::getArguments() const
{
    return m_argumentDescriptors;
}

void SignalType::addSignalInstance(Signal& signal)
{
    lock_guard lock(*this);
    auto it = m_instances.insert(std::make_pair(signal.m_owner, &signal));
    FATAL(it != m_instances.end(), "The SignalType is already in use for signal " << typeid(m_instances.find(signal.m_owner)->second).name())
}

void SignalType::removeSignalInstance(Signal& signal)
{
    lock_guard lock(*this);
#if 1
    auto pv = std::make_pair(signal.m_owner, &signal);
    mox::erase(m_instances, pv);
#else
    const auto it = m_instances.find(signal.m_owner);
    FATAL(it != m_instances.cend(), "Signal corrupted or not in host!")
    m_instances.erase(it, it);
#endif
    signal.m_signalType = nullptr;
    signal.m_owner = nullptr;
}

/******************************************************************************
 *
 */
Signal::Signal(ObjectLock& owner, SignalType& signalType)
    : SharedLock(owner)
    , m_signalType(&signalType)
    , m_owner(&owner)
{
    m_signalType->addSignalInstance(*this);
}

Signal::~Signal()
{
    for_each(m_connections, [](ConnectionSharedPtr connection) { if (connection) connection->m_signal = nullptr; });
    if (m_signalType)
    {
        m_signalType->removeSignalInstance(*this);
    }
}

void Signal::addConnection(ConnectionSharedPtr connection)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);
    m_connections.push_back(connection);
}

void Signal::removeConnection(ConnectionSharedPtr connection)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);

    auto eraser = [&connection](ConnectionSharedPtr conn)
    {
        if (conn != connection)
        {
            return false;
        }

        connection->invalidate();
        return true;
    };
    erase_if(m_connections, eraser);
}

SignalType* Signal::getType()
{
    return m_signalType;
}

const SignalType* Signal::getType() const
{
    return m_signalType;
}

Signal::ConnectionSharedPtr Signal::connect(Callable&& lambda)
{
    FATAL(m_signalType, "Invalid signal")
    return Signal::Connection::create<FunctionConnection>(*this, std::forward<Callable>(lambda));
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, Callable&& slot)
{
    FATAL(m_signalType, "Invalid signal")

    auto connection = ConnectionSharedPtr();
    if (receiver.canConvert<Object*>())
    {
        Object* recv = (Object*)receiver;
        return Signal::Connection::create<ObjectMethodConnection>(*this, *recv, std::forward<Callable>(slot));
    }
    return Signal::Connection::create<MethodConnection>(*this, receiver, std::forward<Callable>(slot));
}

Signal::ConnectionSharedPtr Signal::connect(const Signal& signal)
{
    FATAL(m_signalType, "Invalid signal")
    // Check if the two arguments match.
    if (!signal.getType()->isCompatible(*m_signalType))
    {
        return nullptr;
    }

    return Signal::Connection::create<SignalConnection>(*this, signal);
}

bool Signal::disconnect(const Signal& signal)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);

    auto predicate = [&signal](ConnectionSharedPtr connection)
    {
        auto signalConnection = std::dynamic_pointer_cast<SignalConnection>(connection);
        if (signalConnection && signalConnection->receiverSignal() == &signal)
        {
            connection->invalidate();
            return true;
        }
        return false;
    };
    return erase_if(m_connections, predicate);
}

bool Signal::disconnectImpl(Variant receiver, const Callable& callable)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);

    auto predicate = [&receiver, &callable](ConnectionSharedPtr connection)
    {
        return (connection && connection->disconnect(receiver, callable));
    };
    return erase_if(m_connections, predicate);
}

int Signal::activate(const Callable::ArgumentPack& arguments)
{
    FATAL(m_signalType, "Invalid signal")
    if (m_triggering || m_blocked)
    {
        return 0;
    }

    lock_guard lock(*this);

    FlagScope<true> triggerLock(m_triggering);
    int count = 0;

    auto activator = [&arguments, self = this, &count](ConnectionSharedPtr connection)
    {
        if (!connection)
        {
            return;
        }
        ScopeRelock relock{*self};
        connection->activate(arguments);
        ++count;
    };
    for_each(m_connections, activator);

    return count;
}

} // mox
