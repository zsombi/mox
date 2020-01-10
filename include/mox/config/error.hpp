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

#ifndef ERROR_HPP
#define ERROR_HPP

#include <mox/config/platform_config.hpp>
#include <exception>

namespace mox
{

enum class ExceptionType
{
    InvalidArgument,
    InvalidThreadOwnershipChange,
    DetachedThread,
    AttempThreadJoinWithin,
    MetatypeNotRegistered,
    BadTypeConversion,
    MissingPropertyDefaultValueProvider,
    AttempWriteReadOnlyProperty,
    ValueProviderNotAttached,
    ValueProviderAlreadyAttached,
    PropertyHasDefaultValueProvider,
    PropertyHasExclusiveValueProvider
};

class MOX_API Exception : public std::exception
{
    ExceptionType m_type;
public:
    explicit Exception(ExceptionType type);
    const char * what() const EXCEPTION_NOEXCEPT override;

    constexpr ExceptionType type()
    {
        return m_type;
    }
};

template <ExceptionType Type>
void throwIf(bool test)
{
    if (test)
    {
        throw Exception(Type);
    }
}

} // namespace mox

#endif // ERROR_HPP
