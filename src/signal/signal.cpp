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
#include <mox/object.hpp>

#include <mox/event_handling/event_loop.hpp>

namespace mox
{

/******************************************************************************
 *
 */
static SignalDescriptorBase::TUuid nextUuid()
{
    static SignalDescriptorBase::TUuid uuidPool = 0u;
    return ++uuidPool;
}

SignalDescriptorBase::SignalDescriptorBase(const VariantDescriptorContainer& arguments)
    : arguments(arguments)
    , uuid(nextUuid())
{
}

bool SignalDescriptorBase::operator==(const SignalDescriptorBase& other) const
{
    return uuid == other.uuid;
}

/******************************************************************************
 * SignalId
 */
Signal::SignalId::SignalId(SignalHostConcept& host, Signal& signal, const SignalDescriptorBase& descriptor)
    : SharedLock<ObjectLock>(signal)
    , m_host(host)
    , m_signal(&signal)
    , m_descriptor(descriptor)
{
    m_host.registerSignal(*this);
}

Signal::SignalId::~SignalId()
{
    m_host.removeSignal(*this);
}

Signal& Signal::SignalId::getSignal() const
{
    FATAL(m_signal, "Invlid signal queried!")
    return *m_signal;
}

bool Signal::SignalId::isValid() const
{
    return m_signal != nullptr;
}

bool Signal::SignalId::isA(const SignalDescriptorBase &descriptor) const
{
    return &m_descriptor == &descriptor;
}

/******************************************************************************
 *
 */
Signal::Connection::Connection(Signal& signal)
    : m_signal(&signal)
    , m_passConnectionObject(false)
{
}

void Signal::Connection::reset()
{
    m_signal = nullptr;
    m_connectionType = Type::Deferred;
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

/******************************************************************************
 *
 */
FunctionConnection::FunctionConnection(Signal& signal, Callable&& callable)
    : BaseClass(signal)
    , m_slot(std::forward<Callable>(callable))
{
    m_connectionType = Type::ConnectedToCallable;
    m_passConnectionObject = !m_slot.descriptors().empty() && m_slot.descriptors()[0].type == metaType<Signal::ConnectionSharedPtr>();
}

bool FunctionConnection::compare(const Callable& callable) const
{
    return m_slot == callable;
}

void FunctionConnection::activate(Callable::ArgumentPack& args)
{
    m_slot.apply(prepareActivation(args));
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
    m_connectionType = Type::ConnectedToMethod;
}

bool MethodConnection::compare(Variant receiver, const Callable& callable) const
{
    return (m_receiver.metaType() == receiver.metaType()) && FunctionConnection::compare(callable);
}

void MethodConnection::activate(Callable::ArgumentPack& args)
{
    try
    {
        Object* receiver = m_receiver;
        if (receiver && receiver->threadData() != ThreadData::thisThreadData())
        {
            // Async!!
            postEvent<DeferredSignalEvent>(receiver->shared_from_this(), shared_from_this(), args);
            return;
        }
    }
    catch (...) {}
    m_slot.apply(prepareActivation(args).setInstance(m_receiver));
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
    m_connectionType = Type::ConnectedToMetaMethod;
    m_passConnectionObject = !m_slot->descriptors().empty() && m_slot->descriptors()[0].type == metaType<Signal::ConnectionSharedPtr>();
}

bool MetaMethodConnection::compare(Variant receiver, const Callable& callable) const
{
    return (m_receiver.metaType() == receiver.metaType()) && (*m_slot == callable);
}

void MetaMethodConnection::activate(Callable::ArgumentPack& args)
{
    try
    {
        Object* receiver = m_receiver;
        if (receiver && receiver->threadData() != ThreadData::thisThreadData())
        {
            // Async!!
            postEvent<DeferredSignalEvent>(receiver->shared_from_this(), shared_from_this(), args);
            return;
        }
    }
    catch (...) {}
    m_slot->apply(prepareActivation(args).setInstance(m_receiver));
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
    m_connectionType = Type::ConnectedToSignal;
}

void SignalConnection::activate(Callable::ArgumentPack& args)
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
SignalHostConcept::SignalHostConcept()
{
}

SignalHostConcept::~SignalHostConcept()
{
}

size_t SignalHostConcept::registerSignal(Signal::SignalId& signal)
{
    lock_guard lock(signal);
    m_signals.insert(&signal);
    return m_signals.size() - 1;
}

void SignalHostConcept::removeSignal(Signal::SignalId& signal)
{
    lock_guard lock(signal);
    FATAL(signal.isValid(), "Signal already removed")
    m_signals.erase(&signal);
}

/******************************************************************************
 *
 */
Signal::~Signal()
{
    m_connections.forEach([](ConnectionSharedPtr& connection) { if (connection) connection->m_signal = nullptr; });
}

void Signal::addConnection(ConnectionSharedPtr connection)
{
    FATAL(m_id.isValid(), "Invalid signal")
    lock_guard lock(*this);
    m_connections.append(connection);
}

void Signal::removeConnection(ConnectionSharedPtr connection)
{
    FATAL(m_id.isValid(), "Invalid signal")
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

const Signal::SignalId& Signal::id() const
{
    return m_id;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, const MetaClass::Method& metaMethod)
{
    FATAL(m_id.isValid(), "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MetaMethodConnection>(*this, receiver, metaMethod);
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Callable&& lambda)
{
    FATAL(m_id.isValid(), "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, FunctionConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, Callable&& slot)
{
    FATAL(m_id.isValid(), "Invalid signal")
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MethodConnection>(*this, receiver, std::forward<Callable>(slot));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(const Signal& signal)
{
    FATAL(m_id.isValid(), "Invalid signal")
    // Check if the two arguments match.
    if (!signal.id().descriptor().arguments.isInvocableWith(id().descriptor().arguments))
    {
        return nullptr;
    }

    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, SignalConnection>(*this, signal);
    addConnection(connection);
    return connection;
}

bool Signal::disconnect(const Signal& signal)
{
    FATAL(m_id.isValid(), "Invalid signal")
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
    FATAL(m_id.isValid(), "Invalid signal")
    lock_guard lock(*this);
    lock_guard refConnections(m_connections);

    auto predicate = [&receiver, &callable](ConnectionSharedPtr& connection)
    {
        if (!connection)
        {
            return false;
        }

        switch (connection->m_connectionType)
        {
            case Connection::Type::ConnectedToCallable:
            {
                return static_cast<FunctionConnection*>(connection.get())->compare(callable);
            }
            case Connection::Type::ConnectedToMethod:
            {
                return static_cast<MethodConnection*>(connection.get())->compare(receiver, callable);
            }
            case Connection::Type::ConnectedToMetaMethod:
            {
                return static_cast<MetaMethodConnection*>(connection.get())->compare(receiver, callable);
            }
            default:
            {
                return false;
            }
        }
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

int Signal::activate(Callable::ArgumentPack &args)
{
    FATAL(m_id.isValid(), "Invalid signal")
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
        ScopeRelock relock(*self);
        connection->activate(args);
        ++count;
    };
    m_connections.forEach(activator);

    return count;
}

} // mox
