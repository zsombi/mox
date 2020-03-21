/*
 * Copyright (C) 2017-2020 bitWelder
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
#include <mox/core/object.hpp>

#include <mox/core/module/thread_loop.hpp>
#include <stack>

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

} // noname

/******************************************************************************
 *
 */
Signal::Connection::Connection(Signal& signal)
    : m_signal(&signal)
{
}

void Signal::Connection::invalidate()
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
 * FunctionConnection
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
        invalidate();
        return true;
    }
    return false;
}

void FunctionConnection::activate(const Callable::ArgumentPack& args)
{
    ConnectionScope activeConnection(shared_from_this());
    m_slot.apply(args);
}

void FunctionConnection::invalidate()
{
    m_slot.reset();
    Connection::invalidate();
}

/******************************************************************************
 * ObjectMethodConnection
 */
ObjectMethodConnection::ObjectMethodConnection(Signal& signal, Object& receiver, Callable&& method)
    : FunctionConnection(signal, std::forward<Callable>(method))
    , m_receiver(receiver.shared_from_this())
{
}

bool ObjectMethodConnection::disconnect(Variant receiver, const Callable& callable)
{
    auto recv = m_receiver.lock();
    if (!recv)
    {
        return false;
    }
    if (recv.get() == (Object*)receiver)
    {
        return FunctionConnection::disconnect(receiver, callable);
    }
    return false;
}

void ObjectMethodConnection::activate(const Callable::ArgumentPack& args)
{
    auto receiver = m_receiver.lock();
    if (!receiver)
    {
        return;
    }
    if (receiver->threadData() != ThreadData::thisThreadData())
    {
        // Async!!
        ThreadLoop::postEvent<DeferredSignalEvent>(receiver, *this, args);
        return;
    }

    ConnectionScope activeConnection(shared_from_this());
    m_slot.apply(Callable::ArgumentPack(receiver.get(), prepareActivation(args)));
}

void ObjectMethodConnection::invalidate()
{
    m_receiver.reset();
    FunctionConnection::invalidate();
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
    ConnectionScope activeConnection(shared_from_this());
    m_slot.apply(Callable::ArgumentPack(m_receiver, prepareActivation(args)));
}

void MethodConnection::invalidate()
{
    m_receiver.reset();
    FunctionConnection::invalidate();
}

/******************************************************************************
 * ObjectMetaMethodConnection
 */
ObjectMetaMethodConnection::ObjectMetaMethodConnection(Signal& signal, Object& receiver, const Callable& slot)
    : BaseClass(signal)
    , m_receiver(receiver.shared_from_this())
    , m_slot(&slot)
{
}

bool ObjectMetaMethodConnection::disconnect(Variant receiver, const Callable &callable)
{
    auto recv = m_receiver.lock();
    if (!recv)
    {
        return false;
    }
    if ((recv.get() == (Object*)receiver) && (*m_slot == callable))
    {
        invalidate();
        return true;
    }
    return false;
}

void ObjectMetaMethodConnection::activate(const Callable::ArgumentPack& args)
{
    auto receiver = m_receiver.lock();
    if (!receiver)
    {
        invalidate();
        return;
    }
    if (receiver->threadData() != ThreadData::thisThreadData())
    {
        // Async!!
        ThreadLoop::postEvent<DeferredSignalEvent>(receiver, *this, args);
        return;
    }

    ConnectionScope activeConnection(shared_from_this());
    m_slot->apply(Callable::ArgumentPack(receiver.get(), prepareActivation(args)));
}

void ObjectMetaMethodConnection::invalidate()
{
    m_receiver.reset();
    m_slot = nullptr;
    Connection::invalidate();
}

/******************************************************************************
 * MetaMethodConnection
 */
MetaMethodConnection::MetaMethodConnection(Signal& signal, Variant receiver, const Callable& slot)
    : BaseClass(signal)
    , m_receiver(receiver)
    , m_slot(&slot)
{
}

bool MetaMethodConnection::disconnect(Variant receiver, const Callable &callable)
{
    if ((m_receiver.metaType() == receiver.metaType()) && (*m_slot == callable))
    {
        invalidate();
        return true;
    }
    return false;
}

void MetaMethodConnection::activate(const Callable::ArgumentPack& args)
{
    ConnectionScope activeConnection(shared_from_this());
    m_slot->apply(Callable::ArgumentPack(m_receiver, prepareActivation(args)));
}

void MetaMethodConnection::invalidate()
{
    m_receiver.reset();
    m_slot = nullptr;
    Connection::invalidate();
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

void SignalConnection::invalidate()
{
    m_receiverSignal = nullptr;
    Connection::invalidate();
}

} // mox
