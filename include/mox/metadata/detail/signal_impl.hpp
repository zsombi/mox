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

namespace mox {

template <typename... Args>
int Signal::operator()(Args... arguments)
{
    Callable::Arguments argPack(arguments...);
    return activate(argPack);
}

template <class Receiver>
mox::Signal::ConnectionSharedPtr Signal::connect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return nullptr;
    }
    const MetaClass* metaClass = Receiver::StaticMetaClass::get();
    if constexpr (has_dynamic_metaclass_v<Receiver>)
    {
        metaClass = Receiver::getMetaClass();
    }
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const MetaMethod* method) -> bool
    {
        return (method->name() == name) && isCallableWith(*method, this->metaSignal().descriptors());
    };
    const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return nullptr;
    }

    return connect(metaClass->castInstance(const_cast<Receiver*>(&receiver)), *metaMethod);
}

template <typename SlotFunction>
typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, Signal::ConnectionSharedPtr>::type
Signal::connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    typedef typename function_traits<SlotFunction>::object ReceiverType;

    if constexpr (has_static_metaclass_v<ReceiverType>)
    {
        const MetaClass* metaClass = ReceiverType::StaticMetaClass::get();
        if constexpr (has_dynamic_metaclass_v<ReceiverType>)
        {
            metaClass = ReceiverType::getMetaClass();
        }
        auto visitor = [methodAddress = ::address(method), this](const MetaMethod* method) -> bool
        {
            return (method->address() == methodAddress) && isCallableWith(*method, this->metaSignal().descriptors());
        };
        const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
        if (metaMethod)
        {
            return connect(metaClass->castInstance(const_cast<ReceiverType*>(&receiver)), *metaMethod);
        }
    }

    Callable slotCallable(method);
    if (!isCallableWith(slotCallable, metaSignal().descriptors()))
    {
        return nullptr;
    }
    return connect(&receiver, std::forward<Callable>(slotCallable));
}

template <typename Function>
typename std::enable_if<!std::is_base_of_v<Signal, Function>, Signal::ConnectionSharedPtr>::type
Signal::connect(const Function& function)
{
    Callable lambda(function);
    if (!isCallableWith(lambda, metaSignal().descriptors()))
    {
        return nullptr;
    }
    return connect(std::forward<Callable>(lambda));
}


template <typename Receiver>
bool Signal::disconnect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return false;
    }
    const MetaClass* metaClass = Receiver::StaticMetaClass::get();
    if constexpr (has_dynamic_metaclass_v<Receiver>)
    {
        metaClass = Receiver::getMetaClass();
    }
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const MetaMethod* method) -> bool
    {
        return (method->name() == name) && isCallableWith(*method, this->metaSignal().descriptors());
    };
    const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return false;
    }

    return disconnectImpl(metaClass->castInstance(const_cast<Receiver*>(&receiver)), metaMethod->address());
}

template <typename SlotFunction>
typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, bool>::type
Signal::disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    typedef typename function_traits<SlotFunction>::object ReceiverType;

    std::any receiverInstance(&receiver);
    if constexpr (has_static_metaclass_v<ReceiverType>)
    {
        const MetaClass* metaClass = ReceiverType::StaticMetaClass::get();
        if constexpr (has_dynamic_metaclass_v<ReceiverType>)
        {
            metaClass = ReceiverType::getMetaClass();
        }
        receiverInstance = metaClass->castInstance(&receiver);
    }

    return disconnectImpl(receiverInstance, ::address(method));
}

template <typename SlotFunction>
typename std::enable_if<!std::is_base_of_v<Signal, SlotFunction>, bool>::type
Signal::disconnect(const SlotFunction& slot)
{
    return disconnectImpl(std::any(), ::address(slot));
}



namespace decl
{

template <typename... Args>
template <class SignalOwner>
const MetaSignal& Signal<void(Args...)>::getMetaSignal(std::string_view name)
{
    const std::array<ArgumentDescriptor, sizeof... (Args)> des = {{ ArgumentDescriptor::get<Args>()... }};
    const std::vector<ArgumentDescriptor> desArray(des.begin(), des.end());
    auto visitor = [desArray = std::move(desArray), name = std::string(name)](const MetaSignal* signal)
    {
        return (signal->name() == name) && (signal->descriptors() == desArray);
    };
    const MetaSignal* signal = SignalOwner::StaticMetaClass::get()->visitSignals(visitor);
    ASSERT(signal, std::string("Cannot create a signal without a metasignal for ") + std::string(name));
    return *signal;
}

template <typename... Args>
template <class SignalOwner>
Signal<void(Args...)>::Signal(SignalOwner& owner, std::string_view name)
    : mox::Signal(owner, getMetaSignal<SignalOwner>(name))
{
}

} // namespace impl

} // namespace mox

#endif // SIGNAL_IMPL_HPP
