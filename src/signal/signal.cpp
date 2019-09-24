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

namespace mox
{

static AbstractSignalDescriptor::TUuid nextUuid()
{
    static AbstractSignalDescriptor::TUuid uuidPool = 0u;
    return ++uuidPool;
}

AbstractSignalDescriptor::AbstractSignalDescriptor(const VariantDescriptorContainer& arguments)
    : arguments(arguments)
    , uuid(nextUuid())
{
}

bool AbstractSignalDescriptor::operator==(const AbstractSignalDescriptor& other) const
{
    return uuid == other.uuid;
}

/******************************************************************************
 *
 */
Signal::Connection::Connection(Signal& signal)
    : m_signal(signal)
    , m_passConnectionObject(false)
{
}

Signal& Signal::Connection::signal() const
{
    return m_signal;
}

bool Signal::Connection::disconnect()
{
    if (!isConnected())
    {
        return false;
    }

    m_signal.removeConnection(shared_from_this());
    return true;
}

bool Signal::Connection::compare(Variant receiver, const void* funcAddress) const
{
    UNUSED(receiver);
    UNUSED(funcAddress);
    return false;
}

/******************************************************************************
 *
 */
FunctionConnection::FunctionConnection(Signal& signal, Callable&& callable)
    : Signal::Connection(signal)
    , m_slot(callable)
{
    m_passConnectionObject = !m_slot.descriptors().empty() && m_slot.descriptors()[0].type == metaType<Signal::ConnectionSharedPtr>();
}

bool FunctionConnection::compare(Variant, const void *funcAddress) const
{
    return (m_slot.address() == funcAddress);
}

void FunctionConnection::activate(Callable::ArgumentPack& args)
{
    Callable::ArgumentPack copy;
    if (m_passConnectionObject)
    {
        copy.add(shared_from_this());
    }
    copy += args;
    m_slot.apply(copy);
}

void FunctionConnection::reset()
{
    m_slot.reset();
}

/******************************************************************************
 *
 */
MethodConnection::MethodConnection(Signal& signal, Variant receiver, Callable&& callable)
    : FunctionConnection(signal, std::forward<Callable>(callable))
    , m_receiver(receiver)
{
}

bool MethodConnection::compare(Variant receiver, const void *funcAddress) const
{
    return (m_receiver.metaType() == receiver.metaType()) && FunctionConnection::compare(receiver, funcAddress);
}

void MethodConnection::activate(Callable::ArgumentPack& args)
{
    Callable::ArgumentPack copy;
    if (m_passConnectionObject)
    {
        copy.add(shared_from_this());
    }
    copy += args;
    copy.setInstance(m_receiver);

    m_slot.apply(copy);
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
    : Signal::Connection(signal)
    , m_receiver(receiver)
    , m_slot(&slot)
{
    m_passConnectionObject = !m_slot->descriptors().empty() && m_slot->descriptors()[0].type == metaType<Signal::ConnectionSharedPtr>();
}

bool MetaMethodConnection::compare(Variant receiver, const void *funcAddress) const
{
    return (m_receiver.metaType() == receiver.metaType()) && (m_slot->address() == funcAddress);
}

void MetaMethodConnection::activate(Callable::ArgumentPack& args)
{
    Callable::ArgumentPack copy;
    if (m_passConnectionObject)
    {
        copy.add(shared_from_this());
    }
    copy += args;
    copy.setInstance(m_receiver);
    m_slot->apply(copy);
}

void MetaMethodConnection::reset()
{
    m_receiver.reset();
    m_slot = nullptr;
}

/******************************************************************************
 *
 */
SignalConnection::SignalConnection(Signal& sender, const Signal& other)
    : Signal::Connection(sender)
    , m_receiverSignal(const_cast<Signal*>(&other))
{
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
}

/******************************************************************************
 *
 */
SignalHostNotion::~SignalHostNotion()
{
}

int SignalHostNotion::activate(const AbstractSignalDescriptor& descriptor, Callable::ArgumentPack& args)
{
    ScopeLock lock(m_lock);

    for (auto& signal : m_signals)
    {
        if (&signal->descriptor() == &descriptor)
        {
            return const_cast<Signal*>(signal)->activate(args);
        }
    }

    return -1;
}

size_t SignalHostNotion::registerSignal(Signal& signal)
{
    ScopeLock lock(m_lock);
    m_signals.push_back(&signal);
    return m_signals.size() - 1;
}

void SignalHostNotion::removeSignal(Signal& signal)
{
    ScopeLock lock(m_lock);
    FATAL(signal.isValid(), "Signal already removed")
    m_signals[signal.id()] = nullptr;
}

/******************************************************************************
 *
 */
Signal::~Signal()
{
    m_host.removeSignal(*this);
}

void Signal::addConnection(ConnectionSharedPtr connection)
{
    ScopeLock lock(*this);
    m_connections.push_back(connection);
}

void Signal::removeConnection(ConnectionSharedPtr connection)
{
    ScopeLock lock(*this);
    ConnectionList::iterator it, end = m_connections.end();
    for (it = m_connections.begin(); it != end; ++it)
    {
        ConnectionSharedPtr conn = *it;
        if (conn == connection)
        {
            it->reset();
            connection->reset();
            return;
        }
    }
}

SignalHostNotion& Signal::host() const
{
    return m_host;
}

size_t Signal::id() const
{
    return m_id;
}

bool Signal::isValid() const
{
    return m_id != INVALID_SIGNAL;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, const MetaClass::Method& metaMethod)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MetaMethodConnection>(*this, receiver, metaMethod);
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Callable&& lambda)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, FunctionConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, Callable&& slot)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MethodConnection>(*this, receiver, std::forward<Callable>(slot));
    addConnection(connection);
    return connection;
}

Signal::ConnectionSharedPtr Signal::connect(const Signal& signal)
{
    // Check if the two arguments match.
    if (!signal.m_descriptor.arguments.isInvocableWith(m_descriptor.arguments))
    {
        return nullptr;
    }

    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, SignalConnection>(*this, signal);
    addConnection(connection);
    return connection;
}

bool Signal::disconnect(const Signal& signal)
{
    ScopeLock lock(*this);
    ConnectionList::iterator it, end = m_connections.end();

    for (it = m_connections.begin(); it != end; ++it)
    {
        SignalConnectionSharedPtr signalConnection = std::static_pointer_cast<SignalConnection>(*it);
        if (signalConnection && signalConnection->signal() == &signal)
        {
            *it = nullptr;
            signalConnection->reset();
            return true;
        }
    }

    return false;
}

bool Signal::disconnectImpl(Variant receiver, const void* callableAddress)
{
    ScopeLock lock(*this);
    ConnectionList::iterator it, end = m_connections.end();

    for (it = m_connections.begin(); it != end; ++it)
    {
        ConnectionSharedPtr connection = *it;
        if (connection && connection->compare(receiver, callableAddress))
        {
            *it = nullptr;
            connection->reset();
            return true;
        }
    }

    return false;
}

int Signal::activate(Callable::ArgumentPack &args)
{
    if (m_triggering)
    {
        return 0;
    }

    ScopeLock lock(*this);

    FlagScope<true> triggerLock(m_triggering);
    int count = 0;

    for (auto& connection : m_connections)
    {
        if (connection)
        {
            ScopeRelock relock(*this);
            connection->activate(args);
            count++;
        }
    }

    // Compact the connection container by removing the null connections.
    m_connections.erase(std::remove(m_connections.begin(), m_connections.end(), nullptr), m_connections.end());
    return count;
}

} // mox
