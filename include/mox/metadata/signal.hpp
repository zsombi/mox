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

    SignalHost& host() const;

    size_t id() const;

    bool isValid() const;

    SignalConnectionSharedPtr connect(std::any instance, const MetaMethod* slot);

protected:
    SignalBase() = delete;
    SignalBase(const SignalBase&) = delete;
    SignalBase& operator=(const SignalBase&) = delete;

    explicit SignalBase(SignalHost& host);
    void addConnection(SignalConnectionSharedPtr connection);
    void removeConnection(SignalConnectionSharedPtr connection);
    SignalConnectionSharedPtr connect(Callable&& lambda);
    SignalConnectionSharedPtr connect(std::any instance, Callable&& slot);
    SignalConnectionSharedPtr connect(const SignalBase& signal);

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
    const std::array<ArgumentDescriptor, arity> m_argumentDescriptors = {{ ArgumentDescriptor::get<Args>()... }};

public:
    typedef std::vector<ArgumentDescriptor> ArgumentDescriptorContainer;
    typedef Ret(*Signature)(Args...);

    ArgumentDescriptorContainer argumentDescriptors() const
    {
        return ArgumentDescriptorContainer(m_argumentDescriptors.cbegin(), m_argumentDescriptors.cend());
    }

    explicit Signal(SignalHost& owner)
        : SignalBase(owner)
    {
    }

    template <typename SlotFunction>
    SignalConnectionSharedPtr connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction slot)
    {
        Callable slotCallable(slot);
        if (!slotCallable.isInvocableWith(argumentDescriptors()))
        {
            return nullptr;
        }
        return SignalBase::connect(&receiver, std::forward<Callable>(slotCallable));
    }

    template <typename ReceiverSignal>
    typename std::enable_if<std::is_base_of_v<SignalBase, ReceiverSignal>, SignalConnectionSharedPtr>::type connect(const ReceiverSignal& receiverSignal)
    {
        auto thatArgs = receiverSignal.argumentDescriptors();
        auto argMatch = std::mismatch(thatArgs.cbegin(), thatArgs.cend(), m_argumentDescriptors.cbegin(), m_argumentDescriptors.cend());
        if (argMatch.first != thatArgs.end())
        {
            return nullptr;
        }

        return SignalBase::connect(receiverSignal);
    }

    template <typename Function>
    typename std::enable_if<!std::is_base_of_v<SignalBase, Function>, SignalConnectionSharedPtr>::type connect(const Function& function)
    {
        Callable lambda(function);
        if (!lambda.isInvocableWith(argumentDescriptors()))
        {
            return nullptr;
        }
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
        auto visitor = [name = std::forward<std::string_view>(slotName), retType = metaType<Ret>(), descriptors = argumentDescriptors()](const MetaMethod* method) -> bool
        {
            return (method->name() == name) && (method->returnType().type == retType) &&
                    method->isInvocableWith(descriptors);
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
