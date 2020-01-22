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

#include <mox/property/property.hpp>
#include <mox/property/property_type.hpp>

#include <property_p.hpp>

namespace mox
{

PropertyType::PropertyType(VariantDescriptor&& typeDes, PropertyAccess access, std::string_view name)
    : AbstractMetaInfo(name)
    , m_typeDescriptor(typeDes)
    , m_access(access)
{
}

std::string PropertyType::signature() const
{
    return name() + '<' + MetatypeDescriptor::get(m_typeDescriptor.getType()).name() + '>';
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
    FATAL(m_instances.find(PropertyPrivate::get(property)->host) == m_instances.end(), "Property instance already registered!")

    m_instances.insert(std::make_pair((intptr_t)PropertyPrivate::get(property)->host, &property));
}

void PropertyType::removePropertyInstance(Property& property)
{
    lock_guard lock(*this);
    auto pv = std::make_pair((intptr_t)PropertyPrivate::get(property)->host, &property);
    mox::erase(m_instances, pv);
    PropertyPrivate::get(property)->host.reset();
}

Variant PropertyType::get(Instance instance) const
{
    const auto it = m_instances.find(instance);
    if (it != m_instances.cend())
    {
        return it->second->get();
    }
    return Variant();
}

bool PropertyType::set(Instance instance, const Variant& value) const
{
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(m_access == PropertyAccess::ReadOnly);

    auto it = m_instances.find(instance);
    if (it != m_instances.cend())
    {
        it->second->set(value);
        return true;
    }
    return false;
}

} // namespace mox
