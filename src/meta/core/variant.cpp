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

#include <metadata_p.hpp>

#include <mox/meta/core/variant.hpp>
#include <mox/meta/signal/signal.hpp>

#include <algorithm>

namespace std
{

void swap(mox::Variant& lhs, mox::Variant& rhs)
{
    lhs.swap(rhs);
}

}

namespace mox
{

Variant::Variant(const Variant& other)
    : m_data(other.m_data)
{
}

Variant::Variant(Variant&& other)
{
    swap(other);
}

Variant& Variant::operator=(Variant&& other)
{
    Variant(other).swap(*this);
    return *this;
}

Variant& Variant::operator=(const Variant& other)
{
    Variant(other).swap(*this);
    return *this;
}


bool Variant::isValid() const
{
    return m_data && m_data->m_value.has_value();
}

void Variant::reset()
{
    m_data.reset();
}

Metatype Variant::metaType() const
{
    FATAL(m_data, "Variant is not initialized.");
    return m_data->m_typeDescriptor.getType();
}

const VariantDescriptor& Variant::descriptor() const
{
    FATAL(m_data, "Variant is not initialized.");
    return m_data->m_typeDescriptor;
}

void Variant::swap(Variant &other)
{
    std::swap(m_data, other.m_data);
}

bool operator==(const Variant &var1, const Variant &var2)
{
    if (var1.isValid() && var2.isValid() && (var1.m_data->m_typeDescriptor == var2.m_data->m_typeDescriptor))
    {
        return var1.m_data->m_isEqual(*var2.m_data);
    }

    return false;
}

bool operator!=(const Variant &var1, const Variant &var2)
{
    return !operator==(var1, var2);
}


VariantDescriptor::VariantDescriptor(Metatype type, bool ref, bool c)
    : m_type(type)
    , m_isReference(ref)
    , m_isConst(c)
{
}

bool VariantDescriptor::invocableWith(const VariantDescriptor& other) const
{
#if 1
    const bool typeIsSame = (other.m_type == m_type);
    const bool typeIsConvertible = typeIsSame || MetatypeDescriptor::findConverter(other.m_type, m_type);

    if (m_isReference && other.m_isReference)
    {
        // Both are refs.
        if (m_isConst == other.m_isConst)
        {
            if (m_isConst)
            {
                // Both are const, type conversion is allowed.
                return typeIsConvertible;
            }
            else
            {
                // Neither is const, type must match.
                return typeIsSame;
            }
        }
        else
        {
            // Only one is const. The invocation is allowed if this is const, and other is not.
            // Type conversion is allowed.
            return (m_isConst && typeIsConvertible);
        }
    }
    else if (m_isReference)
    {
        // Only this is ref typed. An invoke is doabnle only if oth this and other are const,
        // and the type is convertible.

        return (m_isConst && other.m_isConst && typeIsConvertible);
    }
    else if (other.m_isReference)
    {
        // The other si reference type, this is not. Call is doable only if the other is const,
        // and the type is convertible.
        return other.m_isConst && typeIsConvertible;
    }
    else
    {
        // Neither is ref type. Call is doable if the type is convertible. Const is ignored.
        return typeIsConvertible;
    }
#else
    return ((other.m_type == m_type) || metadata::findConverter(other.m_type, m_type)) &&
            other.m_isReference == m_isReference &&
            other.m_isConst == m_isConst;
#endif
}

bool VariantDescriptor::operator==(const VariantDescriptor& other) const
{
    return (other.m_type == m_type) &&
            other.m_isReference == m_isReference &&
            other.m_isConst == m_isConst;
}

void VariantDescriptor::swap(VariantDescriptor& other)
{
    std::swap(m_type, other.m_type);
    std::swap(m_isReference, other.m_isReference);
    std::swap(m_isConst, other.m_isConst);
}

bool VariantDescriptorContainer::isInvocableWith(const VariantDescriptorContainer &other) const
{
    auto callableStart = m_container.begin();
    auto callableEnd = m_container.cend();

    auto paramStart = other.m_container.cbegin();
    auto paramEnd = other.m_container.cend();

    while (callableStart != callableEnd && paramStart != paramEnd && callableStart->invocableWith(*paramStart))
    {
        ++callableStart, ++paramStart;
    }

    return (callableStart == callableEnd);
}

} // namespace mox
