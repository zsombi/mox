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

#ifndef SIGNAL_IMPL_HPP
#define SIGNAL_IMPL_HPP

namespace mox
{

template <typename... Args>
size_t SignalType<void(Args...)>::operator()(Args... arguments)
{
    Callable::Arguments argPack(arguments...);
    return activate(argPack);
}

template <typename... Args>
template <class Receiver>
Signal::ConnectionSharedPtr SignalType<void(Args...)>::connect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return nullptr;
    }
    const MetaClass* metaClass = Receiver::getStaticMetaClass();
    if constexpr (has_dynamic_metaclass_v<Receiver>)
    {
        metaClass = Receiver::getDynamicMetaClass();
    }
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const MetaMethod* method) -> bool
    {
        return (method->name() == name) && method->isInvocableWith(this->metaSignal().arguments());
    };
    const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return nullptr;
    }

    return Signal::connect(metaClass->castInstance(const_cast<Receiver*>(&receiver)), *metaMethod);
}

template <typename... Args>
template <typename SlotFunction>
typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, Signal::ConnectionSharedPtr>::type
SignalType<void(Args...)>::connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    typedef typename function_traits<SlotFunction>::object ReceiverType;

    if constexpr (has_static_metaclass_v<ReceiverType>)
    {
        const MetaClass* metaClass = ReceiverType::getStaticMetaClass();
        if constexpr (has_dynamic_metaclass_v<ReceiverType>)
        {
            metaClass = ReceiverType::getDynamicMetaClass();
        }
        auto visitor = [methodAddress = ::address(method), this](const MetaMethod* method) -> bool
        {
            return (method->address() == methodAddress) && method->isInvocableWith(this->metaSignal().arguments());
        };
        const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
        if (metaMethod)
        {
            return Signal::connect(metaClass->castInstance(const_cast<ReceiverType*>(&receiver)), *metaMethod);
        }
    }

    Callable slotCallable(method);
    if (!slotCallable.isInvocableWith(metaSignal().arguments()))
    {
        return nullptr;
    }
    return Signal::connect(&receiver, std::forward<Callable>(slotCallable));
}

template <typename... Args>
template <typename ReceiverSignal>
typename std::enable_if<std::is_base_of_v<Signal, ReceiverSignal>, Signal::ConnectionSharedPtr>::type
SignalType<void(Args...)>::connect(const ReceiverSignal& signal)
{
    auto thatArgs = signal.metaSignal().arguments();
    auto thisArgs = metaSignal().arguments();
    auto argMatch = std::mismatch(thatArgs.cbegin(), thatArgs.cend(), thisArgs.cbegin(), thisArgs.cend());
    if (argMatch.first != thatArgs.end())
    {
        return nullptr;
    }

    return Signal::connect(signal);
}

template <typename... Args>
template <typename Function>
typename std::enable_if<!std::is_base_of_v<Signal, Function>, Signal::ConnectionSharedPtr>::type
SignalType<void(Args...)>::connect(const Function& function)
{
    Callable lambda(function);
    if (!lambda.isInvocableWith(metaSignal().arguments()))
    {
        return nullptr;
    }
    return Signal::connect(std::forward<Callable>(lambda));
}

template <typename... Args>
template <typename Receiver>
bool SignalType<void(Args...)>::disconnect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return false;
    }
    const MetaClass* metaClass = Receiver::getStaticMetaClass();
    if constexpr (has_dynamic_metaclass_v<Receiver>)
    {
        metaClass = Receiver::getDynamicMetaClass();
    }
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const MetaMethod* method) -> bool
    {
        return (method->name() == name) && method->isInvocableWith(this->metaSignal().arguments());
    };
    const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return false;
    }

    return Signal::disconnect(metaClass->castInstance(const_cast<Receiver*>(&receiver)), metaMethod->address());
}

template <typename... Args>
template <typename SlotFunction>
typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, bool>::type
SignalType<void(Args...)>::disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    typedef typename function_traits<SlotFunction>::object ReceiverType;

    std::any receiverInstance(&receiver);
    if constexpr (has_static_metaclass_v<ReceiverType>)
    {
        const MetaClass* metaClass = ReceiverType::getStaticMetaClass();
        if constexpr (has_dynamic_metaclass_v<ReceiverType>)
        {
            metaClass = ReceiverType::getDynamicMetaClass();
        }
        receiverInstance = metaClass->castInstance(&receiver);
    }

    return Signal::disconnect(receiverInstance, ::address(method));
}

template <typename... Args>
template <typename SlotFunction>
typename std::enable_if<!std::is_base_of_v<Signal, SlotFunction>, bool>::type
SignalType<void(Args...)>::disconnect(const SlotFunction& slot)
{
    return Signal::disconnect(std::any(), ::address(slot));
}

template <typename... Args>
template <typename SignalClass>
typename std::enable_if<std::is_base_of_v<Signal, SignalClass>, bool>::type
SignalType<void(Args...)>::disconnect(const SignalClass& signal)
{
    return Signal::disconnect(signal);
}

} // namespace mox

#endif // SIGNAL_IMPL_HPP
