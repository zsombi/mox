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

#include <mox/config/error.hpp>
#include <mox/config/deftypes.hpp>

namespace mox
{

Exception::Exception(ExceptionType type)
    : m_type(type)
{
}

const char* Exception::what() const EXCEPTION_NOEXCEPT
{
    switch (m_type)
    {
        case ExceptionType::InvalidArgument:
            return "invalid argument type applied";
        case ExceptionType::InvalidThreadOwnershipChange:
            return "Changing Object's thread ownership at this time is not allowed.";
        case ExceptionType::DetachedThread:
            return "Detached thread!";
        case ExceptionType::AttempThreadJoinWithin:
            return "Joining thread within the thread scope is not possible";
        case ExceptionType::MetatypeNotRegistered:
            return "Tye RTTI has no metatype registered";
        case ExceptionType::BadTypeConversion:
            return "No converter or faulty metatype conversion.";
        case ExceptionType::InvalidProperty:
            return "Accessing invalid property.";
        case ExceptionType::AttempWriteReadOnlyProperty:
            return "Attempt writing a read-only property";
        case ExceptionType::BindingNotAttached:
            return "The property value provider is not attached.";
        case ExceptionType::BindingAlreadyAttached:
            return "The property value provider is already attached.";
        case ExceptionType::AttemptAttachingBindingToReadOnlyProperty:
            return "Attempt attaching a binding to a read-only property.";
        case ExceptionType::BindingLoop:
            return "Binding loop detected!";
        case ExceptionType::InvalidBinding:
            return "The binding is invalid";
        case ExceptionType::WrongBindingTarget:
            return "Wrong binding target";
        case ExceptionType::BindingNotInGroup:
            return "The binding is not in the group.";
    }
    return nullptr;
}

}
