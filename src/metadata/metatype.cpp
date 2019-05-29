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

MetaTypeDescriptor::MetaTypeDescriptor(const char* name, int id, const std::type_info& rtti, bool isEnum, bool isClass)
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

MetaTypeDescriptor::~MetaTypeDescriptor()
{
    free(m_name);
}

const MetaTypeDescriptor* MetaTypeDescriptor::findMetaType(const std::type_info &rtti)
{
    return metadata().findMetaType(rtti);
}

const MetaTypeDescriptor& MetaTypeDescriptor::newMetatype(const std::type_info &rtti, bool isEnum, bool isClass)
{
    const MetaTypeDescriptor* type = findMetaType(rtti);
    if (!type)
    {
        const MetaTypeDescriptor& newType = metadata().addMetaType(nullptr, rtti, isEnum, isClass);
        ASSERT(newType.id() >= TypeId::UserType, "Type not registered in the user space.");
        type = &newType;
    }
    return *type;
}

const MetaTypeDescriptor& MetaTypeDescriptor::get(TypeId typeId)
{
    return metadata().getMetaType(typeId);
}

bool MetaTypeDescriptor::isSupertypeOf(const MetaTypeDescriptor& type) const
{
    if (!isClass() || !type.isClass())
    {
        return false;
    }

    const MetaClass* thisClass = metadata().getMetaClass(m_id);
    ASSERT(thisClass, "No MetaClass for the class type.");

    const MetaClass* typeClass = metadata().getMetaClass(type.id());
    ASSERT(typeClass, "No MetaClass for the class type.");
    return thisClass->isSuperClassOf(*typeClass);
}

bool MetaTypeDescriptor::derivesFrom(const MetaTypeDescriptor& type) const
{
    if (!isClass() || !type.isClass())
    {
        return false;
    }

    const MetaClass* thisClass = metadata().getMetaClass(m_id);
    ASSERT(thisClass, "No MetaClass for the class type.");

    const MetaClass* typeClass = metadata().getMetaClass(type.id());
    ASSERT(typeClass, "No MetaClass for the class type.");
    return thisClass->derivesFrom(*typeClass);
}

bool MetaTypeDescriptor::isValid() const
{
    return m_rtti != nullptr;
}

bool MetaTypeDescriptor::isVoid() const
{
    return isValid() && (m_id == TypeId::Void);
}

MetaTypeDescriptor::TypeId MetaTypeDescriptor::id() const
{
    return m_id;
}

const char* MetaTypeDescriptor::name() const
{
    ASSERT(m_name, "Empty type name");
    return m_name;
}

bool MetaTypeDescriptor::isEnum() const
{
    return m_isEnum;
}

bool MetaTypeDescriptor::isClass() const
{
    return m_isClass;
}

const std::type_info* MetaTypeDescriptor::rtti() const
{
    return m_rtti;
}

//----------------------------
// Converters
bool MetaTypeDescriptor::registerConverterFunction(AbstractConverterSharedPtr converter, TypeId fromType, TypeId toType)
{
    if (!metadata().addConverter(converter, fromType, toType))
    {
        // LOG a warning.
        return false;
    }
    return true;
}

void MetaTypeDescriptor::unregisterConverterFunction(TypeId fromType, TypeId toType)
{
    metadata().removeConverter(fromType, toType);
}

MetaTypeDescriptor::AbstractConverterSharedPtr MetaTypeDescriptor::findConverter(TypeId from, TypeId to)
{
    return metadata().findConverter(from, to);
}

}// namespace mox
