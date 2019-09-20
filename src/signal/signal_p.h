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

#include <mox/signal/signal.hpp>

namespace mox
{

constexpr size_t INVALID_SIGNAL = std::numeric_limits<size_t>::max();

class FunctionConnection : public Signal::Connection
{
protected:
    Callable m_slot;

public:
    FunctionConnection(Signal& signal, Callable&& callable);

    bool compare(Variant receiver, const void* funcAddress) const override;
    bool isConnected() const override
    {
        return m_slot.type() != FunctionType::Invalid;
    }

    void activate(Callable::ArgumentPack& args) override;
    void reset() override;
};

class MethodConnection : public FunctionConnection
{
    Variant m_receiver;

public:
    MethodConnection(Signal& signal, Variant receiver, Callable&& callable);

    bool compare(Variant receiver, const void* funcAddress) const override;
    void activate(Callable::ArgumentPack& args) override;
    void reset() override;
};

class MetaMethodConnection : public Signal::Connection
{
    Variant m_receiver;
    const MetaClass::Method* m_slot;

public:
    const MetaClass::Method* method() const
    {
        return m_slot;
    }

    MetaMethodConnection(Signal& signal, Variant receiver, const MetaClass::Method& slot);

    bool isConnected() const override
    {
        return m_slot && (m_slot->type() != FunctionType::Invalid);
    }
    bool compare(Variant receiver, const void* funcAddress) const override;
    void activate(Callable::ArgumentPack& args) override;
    void reset() override;
};
typedef std::shared_ptr<MetaMethodConnection> MetaMethodConnectionSharedPtr;

class SignalConnection : public Signal::Connection
{
    Signal* m_receiverSignal;

public:

    Signal* signal() const
    {
        return m_receiverSignal;
    }

    SignalConnection(Signal& sender, const Signal& other);

    bool isConnected() const override
    {
        return m_receiverSignal && m_receiverSignal->isValid();
    }
    void activate(Callable::ArgumentPack& args) override;
    void reset() override;
};
typedef std::shared_ptr<SignalConnection> SignalConnectionSharedPtr;

} // mox

#endif // SIGNAL_P_H
