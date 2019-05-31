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

#include <mox/metadata/signal.hpp>

namespace mox
{

SignalConnection::SignalConnection(std::any receiver)
    : receiver(receiver)
    , isConnectedToSignal(false)
{
}

struct CallableConnection : SignalConnection
{
    Callable slot;

    CallableConnection(std::any receiver, Callable&& callable)
        : SignalConnection(receiver)
        , slot(callable)
    {
    }

    CallableConnection(Callable&& callable)
        : SignalConnection(std::any())
        , slot(callable)
    {
    }
};

struct MetaMethodConnection : SignalConnection
{
    const MetaMethod* slot;

    MetaMethodConnection(std::any receiver, const MetaMethod* slot)
        : SignalConnection(receiver)
        , slot(slot)
    {
    }
};


SignalHost::~SignalHost()
{
}


SignalBase::SignalBase(SignalHost& host)
    : m_host(host)
{
}

SignalConnectionSharedPtr SignalBase::connect(Callable&& lambda)
{
    SignalConnectionSharedPtr connection = SignalConnectionSharedPtr(new CallableConnection(std::forward<Callable>(lambda)));
    m_host.m_connections.push_back(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(std::any instance, Callable&& slot)
{
    SignalConnectionSharedPtr connection = SignalConnectionSharedPtr(new CallableConnection(instance, std::forward<Callable>(slot)));
    m_host.m_connections.push_back(connection);
    return connection;
}

SignalConnectionSharedPtr SignalBase::connect(std::any instance, const MetaMethod* slot)
{
    SignalConnectionSharedPtr connection = SignalConnectionSharedPtr(new MetaMethodConnection(instance, slot));
    m_host.m_connections.push_back(connection);
    return connection;
}


} // mox
