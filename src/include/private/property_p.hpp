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

#ifndef PROPERTY_P_HPP
#define PROPERTY_P_HPP

#include <mox/config/pimpl.hpp>

#include <mox/core/meta/signals.hpp>
#include <mox/core/meta/properties.hpp>
#include <mox/core/meta/bindings.hpp>

#include <unordered_set>

namespace mox
{

class ConnectionStorage
{
public:
    DECLARE_PUBLIC(SignalCore, ConnectionStorage);

    explicit ConnectionStorage(SignalCore& pp)
        : p_ptr(&pp)
    {
    }
    using ConnectionContainer = SharedVector<ConnectionPtr>;

    SignalCore* p_ptr = nullptr;
    ConnectionContainer connections;

    void connect(ConnectionPtr connection);

    // Disconnect slots connected to this signal. Locks the signal, and loops through the connections
    // and removes the slot holders from it.
    void disconnectSlots();

    // Disconnects a connection by removing the connection from both the signal and the slot holder.
    void disconnectOne(ConnectionPtr connection);
};

class PropertyCorePrivate
{
public:
    DECLARE_PUBLIC_PTR(PropertyCore)
    PropertyCorePrivate(PropertyCore* pp)
        : p_ptr(pp)
    {
    }
    ~PropertyCorePrivate();

    void addBinding(BindingCore& binding);
    void removeBinding(BindingCore& binding);


    struct ZeroBindingCheck
    {
        bool operator()(BindingPtr binding)
        {
            return !binding || !binding->isAttached();
        }
    };
    struct ZeroBindingSet
    {
        void operator()(BindingPtr& binding)
        {
            if (binding && binding->isAttached())
            {
                binding->detachFromTarget();
            }
            binding.reset();
        }
    };
    using BindingsStorage = SharedVector<BindingPtr, ZeroBindingCheck, ZeroBindingSet>;

    BindingsStorage bindings;
    BindingPtr activeBinding;
};

class BindingCorePrivate
{
public:
    DECLARE_PUBLIC_PTR(BindingCore);

    explicit BindingCorePrivate(BindingCore* pp)
        : p_ptr(pp)
    {
    }

    enum class Status : byte
    {
        Detaching,
        Detached,
        Attaching,
        Attached
    };

    PropertyCore* target = nullptr;
    BindingGroupPtr group;
    BindingPolicy policy = BindingPolicy::DetachOnWrite;
    Status status = Status::Detached;
    AtomicRefCounted<byte> activationCount = 0;
    std::atomic_bool isEnabled = true;
};

}

#endif // PROPERTY_P_HPP
