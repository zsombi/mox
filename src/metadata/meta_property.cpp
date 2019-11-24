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

#include <mox/metadata/metaclass.hpp>
#include <mox/property/property_type.hpp>

namespace mox
{

MetaClass::Property::Property(MetaClass& metaClass, PropertyType& type, std::string_view name)
    : m_ownerClass(metaClass)
    , m_type(type)
    , m_name(name)
{
    m_ownerClass.addProperty(*this);
}

std::string MetaClass::Property::name() const
{
    return m_name;
}

const PropertyType& MetaClass::Property::type() const
{
    return m_type;
}

Variant MetaClass::Property::get(intptr_t instance) const
{
    return m_type.get(instance);
}

bool MetaClass::Property::set(intptr_t instance, const Variant& value) const
{
    return m_type.set(instance, value);
}

} // namespace mox
