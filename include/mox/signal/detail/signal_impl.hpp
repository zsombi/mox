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

#include <mox/metadata/method_type.hpp>
namespace mox
{

/******************************************************************************
 * Metamethod connect.
 */
template <class Receiver>
mox::Signal::ConnectionSharedPtr Signal::connect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return nullptr;
    }
    const auto* metaClass = Receiver::StaticMetaClass::get();
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const auto method) -> bool
    {
        return (method->name() == name) && method->isInvocableWith(this->getType()->getArguments());
    };
    const auto metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return nullptr;
    }

    return connect(Variant(const_cast<Receiver*>(&receiver)), *metaMethod);
}

template <typename Receiver>
bool Signal::disconnect(const Receiver& receiver, const char* methodName)
{
    if constexpr (!has_static_metaclass_v<Receiver>)
    {
        return false;
    }
    const auto metaClass = Receiver::StaticMetaClass::get();
    auto visitor = [name = std::forward<std::string_view>(methodName), this](const auto method) -> bool
    {
        return (method->name() == name) && method->isInvocableWith(this->getType()->getArguments());
    };
    const auto metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        return false;
    }

    return disconnectImpl(Variant(const_cast<Receiver*>(&receiver)), *metaMethod);
}

/******************************************************************************
 * Method connect.
 */
template <typename SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, Signal::ConnectionSharedPtr>
Signal::connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    Callable slotCallable(method);
    if (!slotCallable.isInvocableWith(m_signalType->getArguments()))
    {
        return nullptr;
    }
    return connect(Variant(&receiver), std::forward<Callable>(slotCallable));
}

template <typename SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, bool>
Signal::disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    Variant receiverInstance(&receiver);
    return disconnectImpl(receiverInstance, Callable(method));
}

/******************************************************************************
 * Function and functor.
 */
template <typename Function>
std::enable_if_t<!std::is_base_of_v<Signal, Function>, Signal::ConnectionSharedPtr>
Signal::connect(const Function& slot)
{
    Callable lambda(slot);
    if (!lambda.isInvocableWith(m_signalType->getArguments()))
    {
        return nullptr;
    }
    return connect(std::forward<Callable>(lambda));
}

template <typename Function>
std::enable_if_t<!std::is_base_of_v<Signal, Function>, bool>
Signal::disconnect(const Function& slot)
{
    return disconnectImpl(Variant(), Callable(slot));
}

/******************************************************************************
 * SignalDecl.
 */
template <typename... Arguments>
int SignalDecl<Arguments...>::operator()(Arguments... arguments)
{
    return activate(Callable::ArgumentPack(arguments...));
}

} // namespace mox

#endif // SIGNAL_IMPL_HPP
