// Copyright (C) 2020 bitWelder

#ifndef SIGNAL_CONNECTION_HPP
#define SIGNAL_CONNECTION_HPP

#include <mox/config/memory.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/meta/argument_data.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

/// Template slot specialized on methods.
template <class Method>
class MethodConnection : public Connection
{
    using Receiver = typename function_traits<Method>::object;
    OrderedLock<Lockable> m_locker;
    Receiver* m_receiver = nullptr;
    Method m_slot;

    MethodConnection(SignalCore& sender, Receiver& receiver, Method slot)
        : Connection(sender)
        , m_locker(sender.get(), &receiver)
        , m_receiver(&receiver)
        , m_slot(slot)
    {
        m_locker.unlock();
    }

public:
    static ConnectionPtr create(SignalCore& sender, Receiver& receiver, Method slot)
    {
        return make_polymorphic_shared_ptr<Connection>(new MethodConnection(sender, receiver, slot));
    }

    SlotHolder* getDestination() const override
    {
        return dynamic_cast<SlotHolder*>(m_receiver);
    }

    void lock() override
    {
        m_locker.lock();
    }

    void unlock() override
    {
        m_locker.unlock();
    }
protected:
    void invokeOverride(const PackedArguments &arguments) override
    {
        FATAL(m_receiver, "Invalid receiver on slot");
        auto argPack = arguments.repack<Method>(m_receiver);
        std::apply(m_slot, argPack);
    }

    void invalidateOverride() override
    {
        m_receiver = nullptr;
    }
};

/// Template slot specialized on functions and lambdas.
template <class Function>
class FunctionConnection : public Connection
{
    Function m_slot;

    FunctionConnection(SignalCore& sender, Function slot)
        : Connection(sender)
        , m_slot(slot)
    {
    }

public:
    static ConnectionPtr create(SignalCore& sender, Function slot)
    {
        return make_polymorphic_shared_ptr<Connection>(new FunctionConnection(sender, slot));
    }

protected:
    void invokeOverride(const PackedArguments &arguments) override
    {
        auto argPack = arguments.repack<Function>();
        std::apply(m_slot, argPack);
    }
};

/// Template slot specialized on connected signals.
template <class... Arguments>
class SignalConnection : public Connection
{
    SignalCore& m_receiver;

    SignalConnection(SignalCore& sender, SignalCore& receiver)
        : Connection(sender)
        , m_receiver(receiver)
    {
    }

public:
    static ConnectionPtr create(SignalCore& sender, SignalCore& receiver)
    {
        return make_polymorphic_shared_ptr<Connection>(new SignalConnection(sender, receiver));
    }

protected:
    void invokeOverride(const PackedArguments &arguments) override
    {
        m_receiver.activate(arguments);
    }
};

/******************************************************************************
 * Implementation
 */
template <class... Arguments>
int Signal<Arguments...>::operator()(Arguments... arguments)
{
    auto pack = PackedArguments(std::move(arguments)...);
    return activate(pack);
}

template <class... Arguments>
template <class SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
Signal<Arguments...>::connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    using Object = typename function_traits<SlotFunction>::object;

    static_assert(
        function_traits<SlotFunction>::arity == 0 ||
        function_traits<SlotFunction>::template test_arguments<Arguments...>::value,
        "Incompatible slot signature");

    auto connection = MethodConnection<SlotFunction>::create(*this, receiver, method);
    addConnection(connection);
    if constexpr (std::is_base_of_v<SlotHolder, Object>)
    {
        receiver.addConnection(connection);
    }
    return connection;
}

template <class... Arguments>
template <class SlotFunction>
std::enable_if_t<!std::is_base_of_v<mox::SignalCore, SlotFunction>, ConnectionPtr>
Signal<Arguments...>::connect(const SlotFunction& slot)
{
    static_assert(
        function_traits<SlotFunction>::arity == 0 ||
        function_traits<SlotFunction>::template test_arguments<Arguments...>::value,
        "Incompatible slot signature");
    auto connection = FunctionConnection<SlotFunction>::create(*this, slot);
    addConnection(connection);
    return connection;
}

template <class... Arguments>
template <class... SignalArguments>
ConnectionPtr Signal<Arguments...>::connect(Signal<SignalArguments...>& signal)
{
    static_assert(
        sizeof...(SignalArguments) == 0 ||
        std::is_same_v<std::tuple<Arguments...>, std::tuple<SignalArguments...>>,
        "incompatible signal signature");
    auto connection = SignalConnection<SignalArguments...>::create(*this, signal);
    addConnection(connection);
    return connection;
}

} // mox

#endif // SIGNAL_CONNECTION_HPP
