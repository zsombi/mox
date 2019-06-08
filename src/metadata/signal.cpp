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

SignalBase::Connection::Connection(SignalBase& signal)
    : m_signal(signal)
{
}

bool SignalBase::Connection::disconnect()
{
    if (!isConnected())
    {
        return false;
    }

    m_signal.removeConnection(shared_from_this());
    return true;
}

bool SignalBase::Connection::compare(std::any receiver, const void* funcAddress) const
{
    UNUSED(receiver);
    UNUSED(funcAddress);
    return false;
}


FunctionConnection::FunctionConnection(SignalBase& signal, Callable&& callable)
    : SignalBase::Connection(signal)
    , m_slot(callable)
{
}

bool FunctionConnection::compare(std::any, const void *funcAddress) const
{
    return (m_slot.address() == funcAddress);
}

void FunctionConnection::activate(Callable::Arguments& args)
{
    m_slot.apply(args);
}

void FunctionConnection::reset()
{
    m_slot.reset();
}


MethodConnection::MethodConnection(SignalBase& signal, std::any receiver, Callable&& callable)
    : FunctionConnection(signal, std::forward<Callable>(callable))
    , m_receiver(receiver)
{
}

bool MethodConnection::compare(std::any receiver, const void *funcAddress) const
{
    return (m_receiver.type() == receiver.type()) && FunctionConnection::compare(receiver, funcAddress);
}

void MethodConnection::activate(Callable::Arguments& args)
{
    Callable::Arguments copy(args);
    copy.prepend(m_receiver);
    FunctionConnection::activate(copy);
}

void MethodConnection::reset()
{
    m_receiver.reset();
    FunctionConnection::reset();
}


MetaMethodConnection::MetaMethodConnection(SignalBase& signal, std::any receiver, const MetaMethod& slot)
    : SignalBase::Connection(signal)
    , m_receiver(receiver)
    , m_slot(&slot)
{
}

bool MetaMethodConnection::compare(std::any receiver, const void *funcAddress) const
{
    return (m_receiver.type() == receiver.type()) && (m_slot->address() == funcAddress);
}

void MetaMethodConnection::activate(Callable::Arguments& args)
{
    Callable::Arguments copy(args);
    copy.prepend(m_receiver);
    m_slot->apply(copy);
}

void MetaMethodConnection::reset()
{
    m_receiver.reset();
    m_slot = nullptr;
}


SignalConnection::SignalConnection(SignalBase& sender, const SignalBase& other)
    : SignalBase::Connection(sender)
    , m_receiverSignal(const_cast<SignalBase*>(&other))
{
}

void SignalConnection::activate(Callable::Arguments& args)
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


SignalHost::~SignalHost()
{
}

size_t SignalHost::registerSignal(SignalBase& signal)
{
    ScopeLock lock(m_lock);
    m_signals.push_back(&signal);
    return m_signals.size() - 1u;
}

void SignalHost::removeSignal(SignalBase &signal)
{
    ScopeLock lock(m_lock);
    ASSERT(signal.isValid(), "Signal already removed");
    m_signals[signal.id()] = nullptr;
}

SignalBase::SignalBase(SignalHost& host)
    : m_host(host)
    , m_id(host.registerSignal(*this))
    , m_triggering(false)
{
}

SignalBase::~SignalBase()
{
    m_host.removeSignal(*this);
}

void SignalBase::addConnection(ConnectionSharedPtr connection)
{
    ScopeLock lock(m_host.m_lock);
    m_connections.push_back(connection);
}

void SignalBase::removeConnection(ConnectionSharedPtr connection)
{
    ScopeLock lock(m_host.m_lock);
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

SignalHost& SignalBase::host() const
{
    return m_host;
}

size_t SignalBase::id() const
{
    return m_id;
}

bool SignalBase::isValid() const
{
    return m_id != INVALID_SIGNAL;
}

SignalBase::ConnectionSharedPtr SignalBase::connect(std::any instance, const MetaMethod& slot)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MetaMethodConnection>(*this, instance, slot);
    addConnection(connection);
    return connection;
}

SignalBase::ConnectionSharedPtr SignalBase::connect(Callable&& lambda)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, FunctionConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

SignalBase::ConnectionSharedPtr SignalBase::connect(std::any instance, Callable&& slot)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, MethodConnection>(*this, instance, std::forward<Callable>(slot));
    addConnection(connection);
    return connection;
}

SignalBase::ConnectionSharedPtr SignalBase::connect(const SignalBase& signal)
{
    ConnectionSharedPtr connection = make_polymorphic_shared<Connection, SignalConnection>(*this, signal);
    addConnection(connection);
    return connection;
}

bool SignalBase::disconnect(const SignalBase& signal)
{
    ScopeLock lock(m_host.m_lock);
    ConnectionList::iterator it, end = m_connections.end();

    for (it = m_connections.begin(); it != end; ++it)
    {
        SignalConnectionSharedPtr signalConnection = std::static_pointer_cast<SignalConnection>(*it);
        if (!signalConnection)
        {
            continue;
        }
        if (signalConnection->signal() == &signal)
        {
            *it = nullptr;
            signalConnection->reset();
            return true;
        }
    }

    return false;
}

bool SignalBase::disconnect(std::any receiver, const void* callableAddress)
{
    ScopeLock lock(m_host.m_lock);
    ConnectionList::iterator it, end = m_connections.end();

    for (it = m_connections.begin(); it != end; ++it)
    {
        ConnectionSharedPtr connection = *it;
        if (!connection)
        {
            continue;
        }
        if (connection->compare(receiver, callableAddress))
        {
            *it = nullptr;
            connection->reset();
            return true;
        }
    }

    return false;
}


size_t SignalBase::activate(Callable::Arguments &args)
{
    if (m_triggering)
    {
        return 0;
    }

    FlagScope<true> lock(m_triggering);
    size_t count = 0;

    for (ConnectionList::const_iterator it = m_connections.cbegin(), end = m_connections.cend(); it != end; ++it)
    {
        if (*it)
        {
            (*it)->activate(args);
            count++;
        }
    }

    // Compact the connection container by removing the null connections.
    m_connections.erase(std::remove(m_connections.begin(), m_connections.end(), nullptr), m_connections.end());
    return count;
}


} // mox
