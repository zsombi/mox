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

#include <mox/metadata/variant.hpp>
#include <mox/metadata/metaclass.hpp>

#include "metadata_p.h"

namespace mox
{

Variant::Variant()
{
}

Variant::Variant(const Variant& other)
    : VariantBase(static_cast<const VariantBase&>(other))
{
}

Variant& Variant::operator=(Variant other)
{
    swap(other);
    return *this;
}

Variant::operator bool() const
{
    return (index() > 0);
}

MetaTypeDescriptor::TypeId Variant::type() const
{
    if (!(*this))
    {
        return MetaTypeDescriptor::TypeId::Invalid;
    }

    MetaTypeDescriptor::TypeId id = static_cast<MetaTypeDescriptor::TypeId>(index());
    switch (id)
    {
        case MetaTypeDescriptor::TypeId::MetaObject:
        {
            MetaObject* mo = value<MetaObject*>();
            return mo->getDynamicMetaClass()->metaType();
        }
        default:
        {
            return id;
        }
    }
}

bool Variant::canConvert(MetaTypeDescriptor::TypeId toType) const
{
    return metadata().findConverter(type(), toType) != nullptr;
}


bool operator==(const Variant& v1, const Variant& v2)
{
    return static_cast<const VariantBase&>(v1) == static_cast<const VariantBase&>(v2);
}

} // namespace mox
