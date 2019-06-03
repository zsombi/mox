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

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <list>
#include <type_traits>
#include <vector>
#include <mox/utils/globals.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metamethod.hpp>

#include <mox/utils/function_traits.hpp>

namespace mox
{

class SignalBase;
class SignalHost;

class MOX_API SignalConnection : std::enable_shared_from_this<SignalConnection>
{
public:
    virtual ~SignalConnection() = default;

    const SignalBase& signal() const
    {
        return m_signal;
    }

    std::any receiver() const
    {
        return m_receiver;
    }

    virtual bool isValid() const = 0;

protected:
    explicit SignalConnection(SignalBase& signal, std::any receiver);

    SignalBase& m_signal;
    std::any m_receiver;

    friend class SignalBase;
};
typedef std::shared_ptr<SignalConnection> SignalConnectionSharedPtr;

class MOX_API SignalBase
{
public:
    virtual ~SignalBase() = default;

    SignalConnectionSharedPtr connect(std::any instance, const MetaMethod* slot);

protected:
    SignalBase() = delete;
    explicit SignalBase(SignalHost& host);
    void addConnection(SignalConnectionSharedPtr connection);
    void removeConnection(SignalConnectionSharedPtr connection);
    SignalConnectionSharedPtr connect(Callable&& lambda);
    SignalConnectionSharedPtr connect(std::any instance, Callable&& slot);

    typedef std::list<SignalConnectionSharedPtr> ConnectionList;
    SignalHost& m_host;
    ConnectionList m_connections;
    size_t m_id;

    friend class SignalConnection;
};

class MOX_API SignalHost
{
public:
    explicit SignalHost() = default;
    virtual ~SignalHost();

protected:
    std::vector<const SignalBase*> m_signals;

    size_t registerSignal(SignalBase& signal);

    friend class SignalBase;
};

template <typename SignatureFunction>
class Signal;

template <typename Ret, typename... Args>
class Signal<Ret(Args...)> : public SignalBase
{
    static constexpr size_t arity = sizeof... (Args);
    std::array<ArgumentDescriptor, arity> argumentDescriptors = {{ArgumentDescriptor::get<Args>()...}};

public:
    explicit Signal(SignalHost& owner)
        : SignalBase(owner)
    {
    }

    template <typename SlotFunction>
    SignalConnectionSharedPtr connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction slot)
    {
        Callable slotCallable(slot);
        if (!slotCallable.isInvocableWith(argument_descriptors<Args...>()))
        {
            return nullptr;
        }
        return SignalBase::connect(&receiver, std::forward<Callable>(slotCallable));
    }

    template <typename Function>
    SignalConnectionSharedPtr connect(const Function& function)
    {
        Callable lambda(function);
        return SignalBase::connect(std::forward<Callable>(lambda));
    }

    template <class Receiver>
    SignalConnectionSharedPtr connectMethod(Receiver& receiver, const char* slotName)
    {
        if constexpr (!has_static_metaclass<Receiver>::value)
        {
            return nullptr;
        }
        const MetaClass* metaClass = nullptr;
        if constexpr (has_dynamic_metaclass<Receiver>::value)
        {
            metaClass = Receiver::getDynamicMetaClass();
        }
        else
        {
            metaClass = Receiver::getStaticMetaClass();
        }
        auto visitor = [name = std::forward<std::string_view>(slotName), retType = metaType<Ret>()](const MetaMethod* method) -> bool
        {
            return (method->name() == name) && (method->returnType().type == retType) &&
                    method->isInvocableWith(std::vector<ArgumentDescriptor>(argument_descriptors<Args...>()));
        };
        const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
        if (!metaMethod)
        {
            return nullptr;
        }

        return SignalBase::connect(metaClass->castInstance(&receiver), metaMethod);
    }

};

} // mox

#endif // SIGNAL_HPP
