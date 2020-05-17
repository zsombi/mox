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

#include <mox/core/meta/property/property.hpp>
#include <mox/core/meta/property/property_type.hpp>

#include <private/property_p.hpp>

namespace mox
{

PropertyType::PropertyType(VariantDescriptor&& typeDes, PropertyAccess access, const SignalType& signal, PropertyDataProviderInterface& defaultValue)
    : ChangedSignalType(signal)
    , m_typeDescriptor(typeDes)
    , m_defaultValue(defaultValue)
    , m_access(access)
{
}

PropertyType::~PropertyType()
{
    VariantDescriptor invalid;
    m_typeDescriptor = invalid;
}

PropertyAccess PropertyType::getAccess() const
{
    return m_access;
}

const VariantDescriptor& PropertyType::getValueType() const
{
    return m_typeDescriptor;
}

Variant PropertyType::getDefault() const
{
    return m_defaultValue.getData();
}

} // namespace mox
