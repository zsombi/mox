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

#include "metadata_p.h"
#include <algorithm>
#include <mox/utils/locks.hpp>
#include <mox/utils/string.hpp>
#include <mox/metadata/argument.hpp>
#include <mox/metadata/signal.hpp>

namespace mox
{

MetaData::MetaData()
{
}

MetaData::~MetaData()
{
}

#define ATOMIC_TYPE(name, Type, typeId) \
{ \
    const MetatypeDescriptor& metaType = addMetaType(name, typeid(Type), std::is_enum_v<Type>, std::is_class_v<Type>, std::is_pointer_v<Type>); \
    ASSERT(metaType.id() == typeId, "wrong atomic type registration!"); \
}

void MetaData::initialize()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    // Mark atomic type initialization completed.
    initialized = true;

    ATOMIC_TYPE("void", void, Metatype::Void);
    ATOMIC_TYPE("bool", bool, Metatype::Bool);
    ATOMIC_TYPE("char", char, Metatype::Char);
    ATOMIC_TYPE("byte", byte, Metatype::Byte);
    ATOMIC_TYPE("short", Int16, Metatype::Short);
    ATOMIC_TYPE("word", UInt16, Metatype::Word);
    ATOMIC_TYPE("int", Int32, Metatype::Int);
    ATOMIC_TYPE("uint", UInt32, Metatype::UInt);
    ATOMIC_TYPE("long", long, Metatype::Long);
    ATOMIC_TYPE("ulong", unsigned long, Metatype::ULong);
    ATOMIC_TYPE("int64", Int64, Metatype::Int64);
    ATOMIC_TYPE("uint64", UInt64, Metatype::UInt64);
    ATOMIC_TYPE("float", float, Metatype::Float);
    ATOMIC_TYPE("double", double, Metatype::Double);
    ATOMIC_TYPE("std::string", std::string, Metatype::String);
    ATOMIC_TYPE("literal", std::string_view, Metatype::Literal);
    ATOMIC_TYPE("void*", void*, Metatype::VoidPtr);
    ATOMIC_TYPE("byte*", byte*, Metatype::BytePtr);
    ATOMIC_TYPE("int*", Int32*, Metatype::IntPtr);
    ATOMIC_TYPE("MetaObject", MetaObject, Metatype::MetaObject);
    ATOMIC_TYPE("MetaObject*", MetaObject*, Metatype::MetaObjectPtr);

    registerMetaType<Signal::ConnectionSharedPtr>();

    // Register converters.
    registerConverters();
}

MetaData& metadata()
{
    static MetaData globalMetaData;
    globalMetaData.initialize();
    return globalMetaData;
}

const MetatypeDescriptor& MetaData::addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer)
{
    ScopeLock locker(sync);

    metaTypes.emplace_back(new MetatypeDescriptor(name, int(metaTypes.size()), rtti, isEnum, isClass, isPointer));
    return *metaTypes.back().get();
}

const MetatypeDescriptor* MetaData::findMetaType(const std::type_info& rtti)
{
    ScopeLock locker(sync);

    for (MetaTypeContainer::const_iterator it = metaTypes.cbegin(); it != metaTypes.cend(); ++it)
    {
        const MetatypeDescriptor* type = it->get();
        if (type->rtti()->hash_code() == rtti.hash_code())
        {
            return type;
        }
    }
    return nullptr;
}

MetatypeDescriptor& MetaData::getMetaType(Metatype type)
{
    ScopeLock locker(sync);
    ASSERT(static_cast<size_t>(type) < metaTypes.size(), "Type not registered to be reflectable.");
    return *metaTypes[static_cast<size_t>(type)].get();
}

void MetaData::addMetaClass(const MetaClass& metaClass)
{
    std::string name = MetatypeDescriptor::get(metaClass.metaType()).name();

    ScopeLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(name);
    ASSERT(it == metaClasses.cend(), name + " MetaClass already registered!");

    metaClassRegister.insert(std::make_pair(metaClass.metaType(), &metaClass));
    metaClasses.insert(std::make_pair(name, &metaClass));
}

void MetaData::removeMetaClass(const MetaClass& metaClass)
{
    std::string name = MetatypeDescriptor::get(metaClass.metaType()).name();

    ScopeLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(name);
    if (it != metaClasses.cend())
    {
        metaClasses.erase(it);
    }
    MetaClassTypeRegister::const_iterator cit = metaClassRegister.find(metaClass.metaType());
    if (cit != metaClassRegister.cend())
    {
        metaClassRegister.erase(cit);
    }
}
const MetaClass* MetaData::findMetaClass(std::string_view name)
{
    ScopeLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(std::string(name));
    return it != metaClasses.cend() ? it->second : nullptr;
}

const MetaClass* MetaData::getMetaClass(Metatype metaType)
{
    ScopeLock locker(sync);
    MetaClassTypeRegister::const_iterator it = metaClassRegister.find(metaType);
    return it != metaClassRegister.cend() ? it->second : nullptr;
}

} // namespace mox
