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

#include <mox/metadata/callable.hpp>
#include <type_traits>

namespace mox
{

Callable::invalid_argument::invalid_argument()
{
}

size_t Callable::Arguments::count() const
{
    return size();
}

/// String representation of the exception.
const char* Callable::invalid_argument::what() const _NOEXCEPT
{
    return "invalid argument type applied";
}

FunctionType Callable::type() const
{
    return m_type;
}

bool Callable::isConst() const
{
    return m_isConst;
}

const ArgumentDescriptor& Callable::returnType() const
{
    return m_ret;
}

MetaType::TypeId Callable::classType() const
{
    return m_classType;
}

size_t Callable::argumentCount() const
{
    return m_args.size();
}

const ArgumentDescriptor& Callable::argumentType(size_t index) const
{
    size_t count = argumentCount();
    if (index >= count)
    {
        throw Callable::invalid_argument();
    }
    return m_args[index];
}

std::any Callable::apply(const Arguments& args) const
{
    return m_invoker(args);
}


} // namespace mox
