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

#include "signal_p.h"
#include <mox/utils/locks.hpp>
#include "../metadata/metadata_p.h"
#include <mox/object.hpp>

#include <mox/event_handling/event_loop.hpp>

namespace mox
{

namespace
{

static thread_local std::stack<Signal::ConnectionSharedPtr> threadActiveConnections;

struct ConnectionScope
{
    explicit ConnectionScope(Signal::ConnectionSharedPtr connection)
    {
        threadActiveConnections.push(connection);
    }
    ~ConnectionScope()
    {
        threadActiveConnections.pop();
    }

};

}

/******************************************************************************
 * SignalType
 */
SignalType::SignalType(VariantDescriptorContainer&& args)
    : m_argumentDescriptors(std::forward<VariantDescriptorContainer>(args))
{
}

int SignalType::activate(intptr_t instance, const Callable::ArgumentPack &args) const
{
    lock_guard lock(const_cast<SignalType&>(*this));
    auto it = m_instances.find(instance);
    if (it != m_instances.cend())
    {
        return it->second->activate(args);
    }
    return -1;
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
}

/******************************************************************************
 *
 */
Signal::Connection::Connection(Signal& signal)
    : m_signal(&signal)
{
}

void Signal::Connection::reset()
{
    m_signal = nullptr;
}

Signal* Signal::Connection::signal() const
{
    return m_signal;
}

bool Signal::Connection::disconnect()
{
    if (!isConnected())
    {
        return false;
    }

    if (m_signal)
    {
        m_signal->removeConnection(shared_from_this());
    }
    return true;
}

Signal::ConnectionSharedPtr Signal::Connection::getActiveConnection()
{
    return threadActiveConnections.top();
}

/******************************************************************************
 *
 */
FunctionConnection::FunctionConnection(Signal& signal, Callable&& callable)
    : BaseClass(signal)
    , m_slot(std::forward<Callable>(callable))
{
}

bool FunctionConnection::disconnect(Variant, const Callable &callable)
{
    if (m_slot == callable)
    {
        reset();
        return true;
    }
    return false;
}

void FunctionConnection::activate(const Callable::ArgumentPack& args)
{
    ConnectionScope activeConnection(shared_from_this());
    m_slot.apply(args);
}

void FunctionConnection::reset()
{
    m_slot.reset();
    Connection::reset();
}

/******************************************************************************
 *
 */
MethodConnection::MethodConnection(Signal& signal, Variant receiver, Callable&& callable)
    : FunctionConnection(signal, std::forward<Callable>(callable))
    , m_receiver(receiver)
{
}

bool MethodConnection::disconnect(Variant receiver, const Callable& callable)
{
    if (m_receiver.metaType() == receiver.metaType())
    {
        return FunctionConnection::disconnect(receiver, callable);
    }
    return false;
}

void MethodConnection::activate(const Callable::ArgumentPack& args)
{
    try
    {
        Object* receiver = m_receiver;
        if (receiver && receiver->threadData() != ThreadData::thisThreadData())
        {
            // Async!!
            postEvent<DeferredSignalEvent>(*receiver, *this, args);
            return;
        }
    }
    catch (...) {}

    ConnectionScope activeConnection(shared_from_this());
    m_slot.apply(Callable::ArgumentPack(m_receiver, prepareActivation(args)));
}

void MethodConnection::reset()
{
    m_receiver.reset();
    FunctionConnection::reset();
}

/******************************************************************************
 *
 */
MetaMethodConnection::MetaMethodConnection(Signal& signal, Variant receiver, const MetaClass::Method& slot)
    : BaseClass(signal)
    , m_receiver(receiver)
    , m_slot(&slot)
{
}

bool MetaMethodConnection::disconnect(Variant receiver, const Callable &callable)
{
    if ((m_receiver.metaType() == receiver.metaType()) && (*m_slot == callable))
    {
        reset();
        return true;
    }
    return false;
}

void MetaMethodConnection::activate(const Callable::ArgumentPack& args)
{
    try
    {
        Object* receiver = m_receiver;
        if (receiver && receiver->threadData() != ThreadData::thisThreadData())
        {
            // Async!!
            postEvent<DeferredSignalEvent>(*receiver, *this, args);
            return;
        }
    }
    catch (...) {}

    ConnectionScope activeConnection(shared_from_this());
    m_slot->apply(Callable::ArgumentPack(m_receiver, prepareActivation(args)));
}

void MetaMethodConnection::reset()
{
    m_receiver.reset();
    m_slot = nullptr;
    Connection::reset();
}

/******************************************************************************
 *
 */
SignalConnection::SignalConnection(Signal& sender, const Signal& other)
    : Signal::Connection(sender)
    , m_receiverSignal(const_cast<Signal*>(&other))
{
}

bool SignalConnection::disconnect(Variant, const Callable&)
{
    return false;
}

void SignalConnection::activate(const Callable::ArgumentPack& args)
{
    if (m_receiverSignal)
    {
        m_receiverSignal->activate(args);
    }
}

void SignalConnection::reset()
{
    m_receiverSignal = nullptr;
    Connection::reset();
}

/******************************************************************************
 *
 */
Signal::Signal(intptr_t owner, SignalType& signalType)
    : SharedLock<ObjectLock>(*reinterpret_cast<ObjectLock*>(owner))
    , m_signalType(&signalType)
    , m_owner(owner)
{
    m_signalType->addSignalInstance(*this);
}

Signal::~Signal()
{
    m_connections.forEach([](ConnectionSharedPtr& connection) { if (connection) connection->m_signal = nullptr; });
    if (m_signalType)
    {
        m_signalType->removeSignalInstance(*this);
    }
}

void Signal::addConnection(ConnectionSharedPtr connection)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);
    m_connections.append(connection);
}

void Signal::removeConnection(ConnectionSharedPtr connection)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);
    lock_guard refConnection(m_connections);

    auto predicate = [&connection](ConnectionSharedPtr& conn)
    {
        return (conn == connection);
    };
    auto index = m_connections.findIf(predicate);
    if (index)
    {
        m_connections[*index].reset();
        connection->reset();
    }
}

SignalType* Signal::getType()
{
    return m_signalType;
}

const SignalType* Signal::getType() const
{
    return m_signalType;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, const MetaClass::Method& metaMethod)
{
    FATAL(m_signalType, "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MetaMethodConnection>(*this, receiver, metaMethod);
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Callable&& lambda)
{
    FATAL(m_signalType, "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, FunctionConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, Callable&& slot)
{
    FATAL(m_signalType, "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MethodConnection>(*this, receiver, std::forward<Callable>(slot));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(const Signal& signal)
{
    FATAL(m_signalType, "Invalid signal")
    // Check if the two arguments match.
    if (!signal.getType()->isCompatible(*m_signalType))
    {
        return nullptr;
    }

    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, SignalConnection>(*this, signal);
    addConnection(connection);
    return connection;
}

bool Signal::disconnect(const Signal& signal)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);
    lock_guard refConnections(m_connections);

    auto predicate = [&signal](ConnectionSharedPtr& connection)
    {
        SignalConnectionSharedPtr signalConnection = std::dynamic_pointer_cast<SignalConnection>(connection);
        return (signalConnection && signalConnection->receiverSignal() == &signal);
    };
    auto index = m_connections.findIf(predicate);
    if (index)
    {
        m_connections[*index]->reset();
        m_connections[*index].reset();
        return true;
    }

    return false;
}

bool Signal::disconnectImpl(Variant receiver, const Callable& callable)
{
    FATAL(m_signalType, "Invalid signal")
    lock_guard lock(*this);
    lock_guard refConnections(m_connections);

    auto predicate = [&receiver, &callable](ConnectionSharedPtr& connection)
    {
        if (!connection)
        {
            return false;
        }

        return connection->disconnect(receiver, callable);
    };
    auto index = m_connections.findIf(predicate);
    if (index)
    {
        m_connections[*index].reset();
        return true;
    }

    return false;
}

int Signal::activate(const Callable::ArgumentPack& args)
{
    FATAL(m_signalType, "Invalid signal")
    if (m_triggering)
    {
        return 0;
    }

    lock_guard lock(*this);

    FlagScope<true> triggerLock(m_triggering);
    int count = 0;

    lock_guard refConnections(m_connections);
    auto activator = [&args, self = this, &count](ConnectionSharedPtr& connection)
    {
        if (!connection)
        {
            return;
        }
        ScopeRelock relock{*self};
        connection->activate(args);
        ++count;
    };
    m_connections.forEach(activator);

    return count;
}

} // mox
