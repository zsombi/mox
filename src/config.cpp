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
        case ExceptionType::MissingPropertyDefaultValueProvider:
            return "No default value provider set for the property!";
        case ExceptionType::AttempWriteReadOnlyProperty:
            return "Attempt writing a read-only property";
        case ExceptionType::ActiveValueProvider:
            return "Attempt activating an already active property value provider.";
        case ExceptionType::InactiveValueProvider:
            return "Attempt deactivating an already inactive property value provider.";
        case ExceptionType::ValueProviderNotAttached:
            return "The property value provider is not attached.";
        case ExceptionType::ValueProviderAlreadyAttached:
            return "The property value provider is already attached.";
    }
    return nullptr;
}

}
