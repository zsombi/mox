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
#include <memory>
#include <mutex>
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

class MOX_API SignalBase
{
public:
    class MOX_API Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        virtual ~Connection() = default;
        virtual bool isConnected() const = 0;
        bool disconnect();
        virtual bool compare(std::any receiver, void* funcAddress) const;

    protected:
        explicit Connection(SignalBase& signal);
        virtual void activate(Callable::Arguments& args) = 0;
        virtual void reset() = 0;

        SignalBase& m_signal;

        friend class SignalBase;
    };
    typedef std::shared_ptr<Connection> ConnectionSharedPtr;

    virtual ~SignalBase();

    SignalHost& host() const;

    size_t id() const;

    bool isValid() const;

    ConnectionSharedPtr connect(std::any instance, const MetaMethod* slot);
    ConnectionSharedPtr connect(const SignalBase& signal);

    size_t activate(Callable::Arguments& args);

protected:
    SignalBase() = delete;
    SignalBase(const SignalBase&) = delete;
    SignalBase& operator=(const SignalBase&) = delete;

    explicit SignalBase(SignalHost& host);
    void addConnection(ConnectionSharedPtr connection);
    void removeConnection(ConnectionSharedPtr connection);

    ConnectionSharedPtr connect(Callable&& lambda);
    ConnectionSharedPtr connect(std::any instance, Callable&& slot);

    bool disconnect(std::any receiver, void* callableAddress);
    bool disconnect(const SignalBase& signal);

    typedef std::vector<ConnectionSharedPtr> ConnectionList;

    SignalHost& m_host;
    ConnectionList m_connections;
    size_t m_id;
    bool m_triggering;

    friend class SignalHost;
};

class MOX_API SignalHost
{
public:
    explicit SignalHost() = default;
    virtual ~SignalHost();

protected:
    std::mutex m_lock;
    std::vector<const SignalBase*> m_signals;

    size_t registerSignal(SignalBase& signal);
    void removeSignal(SignalBase& signal);

    friend class SignalBase;
};

template <typename SignatureFunction>
class Signal;

template <typename... Args>
class Signal<void(Args...)> : public SignalBase
{
    static constexpr size_t arity = sizeof... (Args);
    const std::array<ArgumentDescriptor, arity> m_argumentDescriptors = {{ ArgumentDescriptor::get<Args>()... }};

public:
    typedef std::vector<ArgumentDescriptor> ArgumentDescriptorContainer;
    typedef void(*Signature)(Args...);

    ArgumentDescriptorContainer argumentDescriptors() const
    {
        return ArgumentDescriptorContainer(m_argumentDescriptors.cbegin(), m_argumentDescriptors.cend());
    }

    explicit Signal(SignalHost& owner)
        : SignalBase(owner)
    {
    }

    /// Signal emitter.
    size_t operator()(Args... args)
    {
        Callable::Arguments argPack(args...);
        return activate(argPack);
    }

    /// Connects a \a slot that is a method of a \a receiver.
    /// \return If the connection succeeds, the connection object, or nullptr on failure.
    template <typename SlotFunction>
    ConnectionSharedPtr connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction slot)
    {
        typedef typename function_traits<SlotFunction>::object ReceiverType;
        Callable slotCallable(slot);
        if (!slotCallable.isInvocableWith(argumentDescriptors()))
        {
            return nullptr;
        }
        if constexpr (has_static_metaclass<ReceiverType>::value)
        {
            const MetaClass* metaClass = ReceiverType::getStaticMetaClass();
            if constexpr (has_dynamic_metaclass<ReceiverType>::value)
            {
                metaClass = ReceiverType::getDynamicMetaClass();
            }
            return SignalBase::connect(metaClass->castInstance(&receiver), std::forward<Callable>(slotCallable));
        }
        else
        {
            return SignalBase::connect(&receiver, std::forward<Callable>(slotCallable));
        }
    }

    /// Connects a \a receiverSignal to this signal.
    template <typename ReceiverSignal>
    typename std::enable_if<std::is_base_of_v<SignalBase, ReceiverSignal>, ConnectionSharedPtr>::type connect(const ReceiverSignal& receiverSignal)
    {
        auto thatArgs = receiverSignal.argumentDescriptors();
        auto argMatch = std::mismatch(thatArgs.cbegin(), thatArgs.cend(), m_argumentDescriptors.cbegin(), m_argumentDescriptors.cend());
        if (argMatch.first != thatArgs.end())
        {
            return nullptr;
        }

        return SignalBase::connect(receiverSignal);
    }

    /// Connects a function, or a lambda to this signal.
    template <typename Function>
    typename std::enable_if<!std::is_base_of_v<SignalBase, Function>, ConnectionSharedPtr>::type connect(const Function& function)
    {
        Callable lambda(function);
        if (!lambda.isInvocableWith(argumentDescriptors()))
        {
            return nullptr;
        }
        return SignalBase::connect(std::forward<Callable>(lambda));
    }

    /// Connects a metamethod
    template <class Receiver>
    ConnectionSharedPtr connectMethod(Receiver& receiver, const char* method)
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
        auto visitor = [name = std::forward<std::string_view>(method), descriptors = argumentDescriptors()](const MetaMethod* method) -> bool
        {
            return (method->name() == name) && method->isInvocableWith(descriptors);
        };
        const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
        if (!metaMethod)
        {
            return nullptr;
        }

        return SignalBase::connect(metaClass->castInstance(&receiver), metaMethod);
    }

    /// Disconnects a \a slot that is a method of the \a receiver.
    template <typename SlotFunction>
    bool disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction slot)
    {
        typedef typename function_traits<SlotFunction>::object ReceiverType;
        std::any receiverInstance;
        if constexpr (has_static_metaclass<ReceiverType>::value)
        {
            const MetaClass* metaClass = ReceiverType::getStaticMetaClass();
            if constexpr (has_dynamic_metaclass<ReceiverType>::value)
            {
                metaClass = ReceiverType::getDynamicMetaClass();
            }
            receiverInstance = metaClass->castInstance(&receiver);
        }
        else
        {
            receiverInstance = &receiver;
        }
        return SignalBase::disconnect(receiverInstance, ::address(slot));
    }

    /// Disconnects a \a function from this signal.
    template <typename SlotFunction>
    typename std::enable_if<!std::is_base_of_v<SignalBase, SlotFunction>, bool>::type disconnect(const SlotFunction& slot)
    {
        return SignalBase::disconnect(std::any(), ::address(slot));
    }

    /// Disconnects a \a signal from this signal.
    template <typename SignalType>
    typename std::enable_if<std::is_base_of_v<SignalBase, SignalType>, bool>::type disconnect(const SignalType& signal)
    {
        return SignalBase::disconnect(signal);
    }
};

} // mox

#endif // SIGNAL_HPP
