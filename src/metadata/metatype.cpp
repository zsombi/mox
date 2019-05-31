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
#include <mox/metadata/metatype_descriptor.hpp>
#include <string.h>
#include "metadata_p.h"
#include <cxxabi.h>

#include <mox/utils/function_traits.hpp>

namespace mox
{

bool operator ==(const ArgumentDescriptor& arg1, const ArgumentDescriptor& arg2)
{
    return arg1.type == arg2.type &&
            arg1.isPointer == arg2.isPointer &&
            arg1.isReference == arg2.isReference &&
            arg1.isConst == arg2.isConst;
}

bool operator !=(const ArgumentDescriptor& arg1, const ArgumentDescriptor& arg2)
{
    return arg1.type != arg2.type ||
            arg1.isPointer != arg2.isPointer ||
            arg1.isReference != arg2.isReference ||
            arg1.isConst != arg2.isConst;
}


namespace registrar
{

const MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti)
{
    return metadata().findMetaType(rtti);
}

Metatype findMetatype(const std::type_info& rtti)
{
    const MetatypeDescriptor* descriptor = findMetatypeDescriptor(rtti);
    ASSERT(descriptor, std::string("unregistered metatype: ") + rtti.name());
    return descriptor->id();
}

Metatype tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass)
{
    const MetatypeDescriptor* type = findMetatypeDescriptor(rtti);
    if (!type)
    {
        const MetatypeDescriptor& newType = metadata().addMetaType(nullptr, rtti, isEnum, isClass);
        ASSERT(newType.id() >= Metatype::UserType, "Type not registered in the user space.");
        type = &newType;
    }
    return type->id();
}

} // registrar


MetatypeDescriptor::MetatypeDescriptor(const char* name, int id, const std::type_info& rtti, bool isEnum, bool isClass)
    : m_rtti(&rtti)
    , m_id(Metatype(id))
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

MetatypeDescriptor::~MetatypeDescriptor()
{
    free(m_name);
}

bool MetatypeDescriptor::isCustomType()
{
    return m_id >= Metatype::UserType;
}

const MetatypeDescriptor& MetatypeDescriptor::get(Metatype typeId)
{
    return metadata().getMetaType(typeId);
}

bool MetatypeDescriptor::isSupertypeOf(const MetatypeDescriptor& type) const
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

bool MetatypeDescriptor::derivesFrom(const MetatypeDescriptor& type) const
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

bool MetatypeDescriptor::isValid() const
{
    return m_rtti != nullptr;
}

bool MetatypeDescriptor::isVoid() const
{
    return isValid() && (m_id == Metatype::Void);
}

Metatype MetatypeDescriptor::id() const
{
    return m_id;
}

const char* MetatypeDescriptor::name() const
{
    return m_name;
}

bool MetatypeDescriptor::isEnum() const
{
    return m_isEnum;
}

bool MetatypeDescriptor::isClass() const
{
    return m_isClass;
}

const std::type_info* MetatypeDescriptor::rtti() const
{
    return m_rtti;
}

//----------------------------
// Converters
bool MetatypeDescriptor::registerConverterFunction(AbstractConverterSharedPtr converter, Metatype fromType, Metatype toType)
{
    if (!metadata().addConverter(converter, fromType, toType))
    {
        // LOG a warning.
        return false;
    }
    return true;
}

void MetatypeDescriptor::unregisterConverterFunction(Metatype fromType, Metatype toType)
{
    metadata().removeConverter(fromType, toType);
}

MetatypeDescriptor::AbstractConverterSharedPtr MetatypeDescriptor::findConverter(Metatype from, Metatype to)
{
    return metadata().findConverter(from, to);
}

}// namespace mox
