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

#include <mox/meta/property/property.hpp>
#include <mox/meta/property/property_type.hpp>

#include <property_p.hpp>

namespace mox
{

PropertyType::PropertyType(VariantDescriptor&& typeDes, PropertyAccess access, const Variant& defaultValue)
    : m_typeDescriptor(typeDes)
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

void PropertyType::addPropertyInstance(Property& property)
{
    lock_guard lock(*this);
    FATAL(m_instances.find(PropertyPrivate::get(property)->getHost()) == m_instances.end(), "Property instance already registered!")

    m_instances.insert(std::make_pair(PropertyPrivate::get(property)->getHost(), &property));
}

void PropertyType::removePropertyInstance(Property& property)
{
    lock_guard lock(*this);
    auto pv = std::make_pair(PropertyPrivate::get(property)->getHost(), &property);
    mox::erase(m_instances, pv);
}

Variant PropertyType::get(ObjectLock& instance) const
{
    const auto it = m_instances.find(&instance);
    if (it != m_instances.cend())
    {
        return it->second->get();
    }
    return Variant();
}

bool PropertyType::set(ObjectLock& instance, const Variant& value) const
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(m_access == PropertyAccess::ReadOnly);

    auto it = m_instances.find(&instance);
    if (it != m_instances.cend())
    {
        it->second->set(value);
        return true;
    }
    return false;
}

Variant PropertyType::getDefault() const
{
    return m_defaultValue;
}

} // namespace mox
