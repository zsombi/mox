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
#include <metabase_p.hpp>
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

bool SignalType::isCompatible(const SignalType &other) const
{
    return m_argumentDescriptors.isInvocableWith(other.m_argumentDescriptors);
}

const VariantDescriptorContainer& SignalType::getArguments() const
{
    return m_argumentDescriptors;
}

/******************************************************************************
 *
 */
Signal::Signal(MetaBase& owner, const SignalType& signalType)
    : SharedLock(owner)
    , d_ptr(pimpl::make_d_ptr<SignalStorage>(*this, owner, signalType))
{
}

Signal::~Signal()
{
    if (d_ptr)
    {
        d_ptr->destroy();
    }
}

bool Signal::isBlocked() const
{
    return d_ptr->m_blocked;
}
void Signal::setBlocked(bool blocked)
{
    d_ptr->m_blocked = blocked;
}


void Signal::addConnection(ConnectionSharedPtr connection)
{
    FATAL(d_ptr, "Invalid signal")
    lock_guard lock(*this);
    d_ptr->connections.push_back(connection);
}

void Signal::removeConnection(ConnectionSharedPtr connection)
{
    FATAL(d_ptr, "Invalid signal")
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
    erase_if(d_ptr->connections, eraser);
}

const SignalType* Signal::getType() const
{
    return &d_ptr->getType();
}

Signal::ConnectionSharedPtr Signal::connect(Callable&& lambda)
{
    FATAL(d_ptr, "Invalid signal")
    return Signal::Connection::create<FunctionConnection>(*this, std::forward<Callable>(lambda));
}

Signal::ConnectionSharedPtr Signal::connect(Variant receiver, Callable&& slot)
{
    FATAL(d_ptr, "Invalid signal")

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
    FATAL(d_ptr, "Invalid signal")
    // Check if the two arguments match.
    if (!signal.getType()->isCompatible(d_ptr->getType()))
    {
        return nullptr;
    }

    return Signal::Connection::create<SignalConnection>(*this, signal);
}

bool Signal::disconnect(const Signal& signal)
{
    FATAL(d_ptr, "Invalid signal")
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
    return erase_if(d_ptr->connections, predicate);
}

bool Signal::disconnectImpl(Variant receiver, const Callable& callable)
{
    FATAL(d_ptr, "Invalid signal")
    lock_guard lock(*this);

    auto predicate = [&receiver, &callable](ConnectionSharedPtr connection)
    {
        return (connection && connection->disconnect(receiver, callable));
    };
    return erase_if(d_ptr->connections, predicate);
}

int Signal::activate(const Callable::ArgumentPack& arguments)
{
    FATAL(d_ptr, "Invalid signal")
    D();
    if (d->triggering || d->m_blocked)
    {
        return 0;
    }

    lock_guard lock(*this);

    // If the signal has more arguments than it had activated with, return -1. Consider it as signal not found.
    if (arguments.size() < d->type.getArguments().size())
    {
        return -1;
    }

    FlagScope<true> triggerLock(d->triggering);
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
    for_each(d->connections, activator);

    return count;
}

} // mox
