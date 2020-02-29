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

#ifndef SIGNAL_P_H
#define SIGNAL_P_H

#include <mox/meta/signal/signal.hpp>
#include <mox/meta/signal/signal_type.hpp>

namespace mox
{

TUuid nextUuid();

class Object;
using ObjectWeakPtr = std::weak_ptr<Object>;
/******************************************************************************
 * Connect concept
 */
template <typename DerivedConnectionType>
class ConnectionPrivates : public Signal::Connection
{
    DerivedConnectionType* getSelf()
    {
        return static_cast<DerivedConnectionType*>(this);
    }

public:
    explicit ConnectionPrivates(Signal& signal)
        : Signal::Connection(signal)
    {
    }

    Callable::ArgumentPack prepareActivation(const Callable::ArgumentPack& args)
    {
        return args;
    }
};


class FunctionConnection : public ConnectionPrivates<FunctionConnection>
{
    using BaseClass = ConnectionPrivates<FunctionConnection>;

protected:
    Callable m_slot;

public:
    FunctionConnection(Signal& signal, Callable&& callable);

    bool disconnect(Variant receiver, const Callable& callable) override;
    bool isConnected() const override
    {
        return m_slot.type() != FunctionType::Invalid;
    }

    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};


class ObjectMetaMethodConnection : public ConnectionPrivates<ObjectMetaMethodConnection>
{
    using BaseClass = ConnectionPrivates<ObjectMetaMethodConnection>;

    ObjectWeakPtr m_receiver;
    const Callable* m_slot;

public:
    const Callable* method() const
    {
        return m_slot;
    }

    ObjectMetaMethodConnection(Signal& signal, Object& receiver, const Callable& slot);

    bool isConnected() const override
    {
        return m_slot && (m_slot->type() != FunctionType::Invalid);
    }
    bool disconnect(Variant receiver, const Callable& callable) override;
    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};


class MetaMethodConnection : public ConnectionPrivates<MetaMethodConnection>
{
    using BaseClass = ConnectionPrivates<MetaMethodConnection>;

    Variant m_receiver;
    const Callable* m_slot;

public:
    const Callable* method() const
    {
        return m_slot;
    }

    MetaMethodConnection(Signal& signal, Variant receiver, const Callable& slot);

    bool isConnected() const override
    {
        return m_slot && (m_slot->type() != FunctionType::Invalid);
    }
    bool disconnect(Variant receiver, const Callable& callable) override;
    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};

class ObjectMethodConnection : public FunctionConnection
{
    ObjectWeakPtr m_receiver;
public:
    ObjectMethodConnection(Signal& signal, Object& receiver, Callable&& method);

    bool disconnect(Variant receiver, const Callable& callable) override;
    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};

class MethodConnection : public FunctionConnection
{
    Variant m_receiver;

public:
    MethodConnection(Signal& signal, Variant receiver, Callable&& callable);

    bool disconnect(Variant receiver, const Callable& callable) override;
    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};

class SignalConnection : public Signal::Connection
{
    Signal* m_receiverSignal = nullptr;

public:

    Signal* receiverSignal() const
    {
        return m_receiverSignal;
    }

    SignalConnection(Signal& sender, const Signal& other);

    bool isConnected() const override
    {
        return m_receiverSignal && (m_receiverSignal->getType() != nullptr);
    }
    bool disconnect(Variant receiver, const Callable& callable) override;
    void activate(const Callable::ArgumentPack& args) override;
    void invalidate() override;
};

} // mox

#endif // SIGNAL_P_H
