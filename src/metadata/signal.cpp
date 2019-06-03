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

namespace mox
{

SignalConnection::SignalConnection(SignalBase& signal, std::any receiver)
    : m_signal(signal)
    , m_receiver(receiver)
{
}

bool SignalConnection::isConnected() const
{
    for (auto connection : m_signal.m_connections)
    {
        if (connection == shared_from_this())
        {
            return true;
        }
    }
    return false;
}

bool SignalConnection::disconnect()
{
    if (!isConnected())
    {
        return false;
    }

    m_signal.removeConnection(shared_from_this());
    return true;
}


CallableConnection::CallableConnection(SignalBase& signal, std::any receiver, Callable&& callable)
    : SignalConnection(signal, receiver)
    , m_slot(callable)
{
}

CallableConnection::CallableConnection(SignalBase& signal, Callable&& callable)
    : SignalConnection(signal, std::any())
    , m_slot(callable)
{
}


MetaMethodConnection::MetaMethodConnection(SignalBase& signal, std::any receiver, const MetaMethod* slot)
    : SignalConnection(signal, receiver)
    , m_slot(slot)
{
}

SignalReceiverConnection::SignalReceiverConnection(SignalBase& sender, const SignalBase& other)
    : SignalConnection(sender, std::any(&other.host()))
    , m_receiverSignal(other)
{
}


SignalHost::~SignalHost()
{
}

size_t SignalHost::registerSignal(SignalBase& signal)
{
    m_signals.push_back(&signal);
    return m_signals.size() - 1u;
}

SignalBase::SignalBase(SignalHost& host)
    : m_host(host)
    , m_id(host.registerSignal(*this))
{
}

void SignalBase::addConnection(SignalConnectionSharedPtr connection)
{
    m_connections.push_back(connection);
}

void SignalBase::removeConnection(SignalConnectionSharedPtr connection)
{
    ConnectionList::iterator it, end = m_connections.end();
    for (it = m_connections.begin(); it != end; ++it)
    {
        if (*it == connection)
        {
            it->reset();
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

SignalConnectionSharedPtr SignalBase::connect(std::any instance, const MetaMethod* slot)
{
    SignalConnectionSharedPtr connection = make_polymorphic_shared<SignalConnection, MetaMethodConnection>(*this, instance, slot);
    addConnection(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(Callable&& lambda)
{
    SignalConnectionSharedPtr connection = make_polymorphic_shared<SignalConnection, CallableConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(std::any instance, Callable&& slot)
{
    SignalConnectionSharedPtr connection = make_polymorphic_shared<SignalConnection, CallableConnection>(*this, instance, std::forward<Callable>(slot));
    addConnection(connection);
    return std::static_pointer_cast<SignalConnection>(connection);
}

SignalConnectionSharedPtr SignalBase::connect(const SignalBase& signal)
{
    SignalConnectionSharedPtr connection = make_polymorphic_shared<SignalConnection, SignalReceiverConnection>(*this, signal);
    addConnection(connection);
    return connection;
}


} // mox
