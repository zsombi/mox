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

#include "metadata_p.h"

#include <mox/metadata/variant.hpp>
#include <mox/signal/signal.hpp>

#include <algorithm>

namespace mox
{

Variant::Variant(const Variant& other)
    : m_data(other.m_data)
{
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
    return m_data->m_typeDescriptor.type;
}

const VariantDescriptor& Variant::descriptor() const
{
    FATAL(m_data, "Variant is not initialized.");
    return m_data->m_typeDescriptor;
}


VariantDescriptor::VariantDescriptor(Metatype type, bool ref, bool c)
    : type(type)
    , isReference(ref)
    , isConst(c)
{
}

bool VariantDescriptor::invocableWith(const VariantDescriptor& other) const
{
//    const bool typeIsSame = (other.type == type);
//    const bool typeIsConvertible = typeIsSame || metadata::findConverter(other.type, type);

//    if (isReference && other.isReference)
//    {
//        // Both are refs.
//        if (isConst == other.isConst)
//        {
//            if (isConst)
//            {
//                // Both are const, type conversion is allowed.
//                return typeIsConvertible;
//            }
//            else
//            {
//                // Neither is const, type must match.
//                return typeIsSame;
//            }
//        }
//        else
//        {
//            // Only one is const. The invocation is allowed if this is const, and other is not.
//            // Type conversion is allowed.
//            return (isConst && typeIsConvertible);
//        }
//    }
//    else if (isReference)
//    {
//        // Only this is ref typed. An invoke is doabnle only if oth this and other are const,
//        // and the type is convertible.

//        return (isConst && other.isConst && typeIsConvertible);
//    }
//    else if (other.isReference)
//    {
//        // The other si reference type, this is not. Call is doable only if the other is const,
//        // and the type is convertible.
//        return other.isConst && typeIsConvertible;
//    }
//    else
//    {
//        // Neither is ref type. Call is doable if the type is convertible. Const is ignored.
//        return typeIsConvertible;
//    }
    return ((other.type == type) || metadata::findConverter(other.type, type)) &&
            other.isReference == isReference &&
            other.isConst == isConst;
}

bool VariantDescriptor::operator==(const VariantDescriptor& other) const
{
    return (other.type == type) &&
            other.isReference == isReference &&
            other.isConst == isConst;
}

void VariantDescriptor::swap(VariantDescriptor& other)
{
    std::swap(const_cast<Metatype&>(type), const_cast<Metatype&>(other.type));
    std::swap(const_cast<bool&>(isReference), const_cast<bool&>(other.isReference));
    std::swap(const_cast<bool&>(isConst), const_cast<bool&>(other.isConst));
}

bool VariantDescriptorContainer::isInvocableWith(const VariantDescriptorContainer &other) const
{
    auto callableStart = begin();
    auto callableEnd = cend();

    if ((callableStart != callableEnd) && (callableStart->type == metaType<Signal::ConnectionSharedPtr>()))
    {
        ++callableStart;
    }

    auto paramStart = other.cbegin();
    auto paramEnd = other.cend();

    while (callableStart != callableEnd && paramStart != paramEnd && callableStart->invocableWith(*paramStart))
    {
        ++callableStart, ++paramStart;
    }

    return (callableStart == callableEnd);
}

} // namespace mox
