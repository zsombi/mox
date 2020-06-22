/*
 * Copyright (C) 2017-2018 bitWelder
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

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <chrono>
#include <memory>
#include <iostream>

namespace mox
{

/// Creates a polymorphic shared pointer. Wipes the code bloat caused by the shared pointer.
template <typename BaseType, typename Type, typename... Args>
std::shared_ptr<Type> make_polymorphic_shared(Args&&... args)
{
    static_assert(std::is_base_of_v<BaseType, Type>, "BaseType is not a superclass of Type.");
    auto shared = std::shared_ptr<BaseType>{std::unique_ptr<BaseType>(std::make_unique<Type>(std::forward<Args>(args)...))};
    return std::static_pointer_cast<Type>(shared);
}

template <typename BaseType, typename Type>
std::shared_ptr<Type> make_polymorphic_shared_ptr(Type* type)
{
    static_assert(std::is_base_of_v<BaseType, Type>, "BaseType is not a superclass of Type.");
    auto shared = std::shared_ptr<BaseType>{std::unique_ptr<BaseType>(type)};
    return std::static_pointer_cast<Type>(shared);
}

template <class To, class From>
std::shared_ptr<To> as_shared(From* from)
{
    return from ? std::static_pointer_cast<To>(from->shared_from_this()) : nullptr;
}

template <class To, class From>
std::shared_ptr<To> as_shared(std::shared_ptr<From> from)
{
    return std::static_pointer_cast<To>(from);
}

} // mox

#endif // GLOBALS
