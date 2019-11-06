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

size_t Callable::ArgumentPack::count() const
{
    return size();
}

Callable::ArgumentPack& Callable::ArgumentPack::operator+=(const ArgumentPack &other)
{
    insert(end(), other.begin(), other.end());
    return *this;
}

/// String representation of the exception.
const char* Callable::invalid_argument::what() const EXCEPTION_NOEXCEPT
{
    return "invalid argument type applied";
}

Callable::Callable(Callable&& other)
{
    swap(other);
}

Callable& Callable::operator=(Callable&& other)
{
    swap(other);
    return *this;
}

bool Callable::operator==(const Callable& other) const
{
    return m_invoker.target_type() == other.m_invoker.target_type();
}

void Callable::swap(Callable& other)
{
    m_invoker.swap(other.m_invoker);
    m_ret.swap(other.m_ret);
    m_args.swap(other.m_args);
    std::swap(m_classType, other.m_classType);
    std::swap(m_type, other.m_type);
    std::swap(m_isConst, other.m_isConst);
}

FunctionType Callable::type() const
{
    return m_type;
}

bool Callable::isConst() const
{
    return m_isConst;
}

const VariantDescriptor& Callable::returnType() const
{
    return m_ret;
}

Metatype Callable::classType() const
{
    return m_classType;
}

size_t Callable::argumentCount() const
{
    return m_args.size();
}

const VariantDescriptor& Callable::argumentType(size_t index) const
{
    size_t count = argumentCount();
    if (index >= count)
    {
        throw Callable::invalid_argument();
    }
    return m_args[index];
}

const VariantDescriptorContainer& Callable::descriptors() const
{
    return m_args;
}

bool Callable::isInvocableWith(const VariantDescriptorContainer& arguments) const
{
    return m_args.isInvocableWith(arguments);
}

Variant Callable::apply(const ArgumentPack& args) const
{
    return m_invoker(args);
}

void Callable::reset()
{
    m_args.clear();
    m_classType = Metatype::Invalid;
    m_type = FunctionType::Invalid;
}


} // namespace mox
