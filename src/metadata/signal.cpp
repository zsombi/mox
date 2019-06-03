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

SignalConnectionSharedPtr SignalBase::connect(Callable&& lambda)
{
    SignalConnectionSharedPtr connection = std::make_shared<CallableConnection>(*this, std::forward<Callable>(lambda));
    addConnection(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(std::any instance, Callable&& slot)
{
    SignalConnectionSharedPtr connection = std::make_shared<CallableConnection>(*this, instance, std::forward<Callable>(slot));
    addConnection(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(std::any instance, const MetaMethod* slot)
{
    SignalConnectionSharedPtr connection = std::make_shared<MetaMethodConnection>(*this, instance, slot);
    addConnection(connection);
    return connection;
}


} // mox
