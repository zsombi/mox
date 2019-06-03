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

#include <mox/metadata/signal.hpp>

namespace mox
{

constexpr size_t INVALID_SIGNAL = std::numeric_limits<size_t>::max();

class CallableConnection : public SignalConnection
{
    Callable m_slot;

public:
    CallableConnection(SignalBase& signal, std::any receiver, Callable&& callable);
    CallableConnection(SignalBase& signal, Callable&& callable);

    bool isValid() const override
    {
        return m_slot.type() != FunctionType::Invalid;
    }
};

class MetaMethodConnection : public SignalConnection
{
    const MetaMethod* m_slot;

public:
    MetaMethodConnection(SignalBase& signal, std::any receiver, const MetaMethod* slot);

    bool isValid() const override
    {
        return m_slot->type() != FunctionType::Invalid;
    }
};

class SignalReceiverConnection : public SignalConnection
{
    const SignalBase& m_receiverSignal;

public:
    SignalReceiverConnection(SignalBase& sender, const SignalBase& other);

    bool isValid() const override
    {
        return m_receiverSignal.isValid();
    }
};

} // mox

#endif // SIGNAL_P_H
