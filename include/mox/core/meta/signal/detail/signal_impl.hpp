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

#include <mox/config/memory.hpp>

namespace mox
{

template <class Derived, typename... Arguments>
Signal::ConnectionSharedPtr Signal::Connection::create(Signal& sender, Arguments&&... args)
{
    auto connection = make_polymorphic_shared<Connection, Derived>(sender, std::forward<Arguments>(args)...);
    sender.addConnection(connection);
    return connection;
}

/******************************************************************************
 * Method connect.
 */
template <typename SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, Signal::ConnectionSharedPtr>
Signal::connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method)
{
    Callable slotCallable(method);
    if (!slotCallable.isInvocableWith(getType()->getArguments()))
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
    if (!lambda.isInvocableWith(getType()->getArguments()))
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

template <typename... Arguments>
int Signal::operator()(Arguments... arguments)
{
#ifdef DEBUG
    FATAL(getType()->getArguments().isInvocableWithArgumentTypes<Arguments...>(), "Signal arguments and signal type arguments mismatch");
#endif
    return activate(Callable::ArgumentPack(arguments...));
}

} // namespace mox

#endif // SIGNAL_IMPL_HPP
