/*
 * Copyright (C) 2017-2018 bitWelder
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

#include <mox/metadata/metatype.hpp>
#include <string.h>
#include "metadata_p.h"
#include <cxxabi.h>

namespace mox
{

MetaType::MetaType(const char* name, int id, const std::type_info& rtti, bool isEnum, bool isClass)
    : m_rtti(&rtti)
    , m_id(TypeId(id))
    , m_isEnum(isEnum)
    , m_isClass(isClass)
{
    if (name)
    {
        // Use the name to override RTTI name.
        m_name = strdup(name);
    }
    else
    {
        // Try to deduct the namespace from the RTTI name.
        int status = 0;
        m_name = abi::__cxa_demangle(rtti.name(), nullptr, nullptr, &status);
    }
    ASSERT(m_name, "Null name type!");
}

MetaType::~MetaType()
{
    free(m_name);
}

const MetaType* MetaType::findMetaType(const std::type_info &rtti)
{
    return metadata().findMetaType(rtti);
}

const MetaType& MetaType::newMetatype(const std::type_info &rtti, bool isEnum, bool isClass)
{
    const MetaType* type = findMetaType(rtti);
    if (!type)
    {
        const MetaType& newType = metadata().addMetaType(nullptr, rtti, isEnum, isClass);
        ASSERT(newType.id() >= TypeId::UserType, "Type not registered in the user space.");
        type = &newType;
    }
    return *type;
}

const MetaType& MetaType::get(TypeId typeId)
{
    return metadata().getMetaType(typeId);
}

bool MetaType::isValid() const
{
    return m_rtti != nullptr;
}

bool MetaType::isVoid() const
{
    return isValid() && (m_id == TypeId::Void);
}

MetaType::TypeId MetaType::id() const
{
    return m_id;
}

const char* MetaType::name() const
{
    ASSERT(m_name, "Empty type name");
    return m_name;
}

bool MetaType::isEnum() const
{
    return m_isEnum;
}

bool MetaType::isClass() const
{
    return m_isClass;
}

const std::type_info* MetaType::rtti() const
{
    return m_rtti;
}

}// namespace Mox
