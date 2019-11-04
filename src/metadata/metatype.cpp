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
#include <mox/signal/signal.hpp>

#include <sstream>

namespace mox
{


type_not_registered::type_not_registered(const std::type_info& rtti)
{
    int status = 0;
    char* abiName = abi::__cxa_demangle(rtti.name(), nullptr, nullptr, &status);
    m_message = "Type '" + std::string(abiName) + "' not registered as metatype.";
    free(abiName);
}
const char* type_not_registered::what() const EXCEPTION_NOEXCEPT
{
    return m_message.c_str();
}


MetatypeDescriptor::MetatypeDescriptor(std::string_view name, int id, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer)
    : m_rtti(&rtti)
    , m_id(Metatype(id))
    , m_isEnum(isEnum)
    , m_isClass(isClass)
    , m_isPointer(isPointer)
{
    if (!name.empty())
    {
        // Use the name to override RTTI name.
        m_name = strdup(name.data());
    }
    else
    {
        // Try to deduct the namespace from the RTTI name.
        int status = 0;
        m_name = abi::__cxa_demangle(rtti.name(), nullptr, nullptr, &status);
    }
    FATAL(m_name, "Null name type!");

    TRACE("New metatype: " << m_name);
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
    return MetaData::getMetaType(typeId);
}

bool MetatypeDescriptor::isSupertypeOf(const MetatypeDescriptor& type) const
{
    if (!isClass() || !type.isClass())
    {
        return false;
    }

    const MetaClass* thisClass = MetaData::getMetaClass(m_id);
    FATAL(thisClass, "No MetaClass for the class type.");

    const MetaClass* typeClass = MetaData::getMetaClass(type.id());
    FATAL(typeClass, "No MetaClass for the class type.");
    return thisClass->isSuperClassOf(*typeClass);
}

bool MetatypeDescriptor::derivesFrom(const MetatypeDescriptor& type) const
{
    if (!isClass() || !type.isClass())
    {
        return false;
    }

    const MetaClass* thisClass = MetaData::getMetaClass(m_id);
    FATAL(thisClass, "No MetaClass for the class type.");

    const MetaClass* typeClass = MetaData::getMetaClass(type.id());
    FATAL(typeClass, "No MetaClass for the class type.");
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

bool MetatypeDescriptor::isPointer() const
{
    return m_isPointer;
}

const std::type_info* MetatypeDescriptor::rtti() const
{
    return m_rtti;
}

MetatypeConverter* MetatypeDescriptor::findConverterTo(Metatype target)
{
    auto i = m_converters.find(target);
    if (i == m_converters.cend())
        return nullptr;
    else
        return i->second.get();
}

bool MetatypeDescriptor::addConverter(MetatypeConverterPtr&& converter, Metatype target)
{
    if (m_converters.find(target) == m_converters.end())
    {
        m_converters.insert(std::make_pair(target, std::forward<MetatypeConverterPtr>(converter)));
        return true;
    }
    return false;
}

#define ATOMIC_TYPE(name, Type, typeId) \
{ \
    const MetatypeDescriptor& metaType = metaData.addMetaType(name, typeid(Type), std::is_enum_v<Type>, std::is_class_v<Type>, std::is_pointer_v<Type>); \
    FATAL(metaType.id() == typeId, "wrong atomic type registration!"); \
}

void registerAtomicTypes(MetaData& metaData)
{
    ATOMIC_TYPE("void", void, Metatype::Void)
    ATOMIC_TYPE("bool", bool, Metatype::Bool)
    ATOMIC_TYPE("char", char, Metatype::Char)
    ATOMIC_TYPE("byte", byte, Metatype::Byte)
    ATOMIC_TYPE("short", int16_t, Metatype::Short)
    ATOMIC_TYPE("word", uint16_t, Metatype::Word)
    ATOMIC_TYPE("int", int32_t, Metatype::Int32)
    ATOMIC_TYPE("uint", uint32_t, Metatype::UInt32)
    ATOMIC_TYPE("int64", int64_t, Metatype::Int64)
    ATOMIC_TYPE("uint64", uint64_t, Metatype::UInt64)
    ATOMIC_TYPE("float", float, Metatype::Float)
    ATOMIC_TYPE("double", double, Metatype::Double)
    ATOMIC_TYPE("std::string", std::string, Metatype::String)
    ATOMIC_TYPE("literal", std::string_view, Metatype::Literal)
    ATOMIC_TYPE("void*", void*, Metatype::VoidPtr)
    ATOMIC_TYPE("byte*", byte*, Metatype::BytePtr)

#ifdef LONG_SYNONIM_OF_UINT64
    metaData.synonymTypes.push_back(std::make_pair(&typeid(long), Metatype::Int64));
    metaData.synonymTypes.push_back(std::make_pair(&typeid(unsigned long), Metatype::UInt64));
#endif
}

}// namespace mox
