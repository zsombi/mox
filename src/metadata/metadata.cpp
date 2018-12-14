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
    const MetaType& metaType = addMetaType(name, typeid(Type), std::is_enum<Type>(), std::is_class<Type>()); \
    ASSERT(metaType.id() == typeId, "wrong atomic type registration!"); \
}

void MetaData::initialize()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    ATOMIC_TYPE("bool", bool, MetaType::TypeId::Bool);
    ATOMIC_TYPE("char", char, MetaType::TypeId::Char);
    ATOMIC_TYPE("byte", byte, MetaType::TypeId::Byte);
    ATOMIC_TYPE("short", short, MetaType::TypeId::Short);
    ATOMIC_TYPE("word", unsigned short, MetaType::TypeId::Word);
    ATOMIC_TYPE("int", int, MetaType::TypeId::Int);
    ATOMIC_TYPE("uint", unsigned int, MetaType::TypeId::UInt);
    ATOMIC_TYPE("long", long, MetaType::TypeId::Long);
    ATOMIC_TYPE("ulong", unsigned long, MetaType::TypeId::ULong);
    ATOMIC_TYPE("int64", long long, MetaType::TypeId::Int64);
    ATOMIC_TYPE("uint64", unsigned long long, MetaType::TypeId::UInt64);
    ATOMIC_TYPE("float", float, MetaType::TypeId::Float);
    ATOMIC_TYPE("double", double, MetaType::TypeId::Double);
    ATOMIC_TYPE("size_t", size_t, MetaType::TypeId::Size);
    // Weirdo void type.
    ATOMIC_TYPE("void", void, MetaType::TypeId::Void);
    // Standard lib types.
    ATOMIC_TYPE("std::string", std::string, MetaType::TypeId::StdString);

    initialized = true;
}

MetaData& metadata()
{
    static MetaData globalMetaData;
    globalMetaData.initialize();
    return globalMetaData;
}

const MetaType& MetaData::addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass)
{
    MutexLock locker(sync);

    metaTypes.emplace_back(new MetaType(name, int(metaTypes.size()), rtti, isEnum, isClass));
    return *metaTypes.back().get();
}

const MetaType* MetaData::findMetaType(const std::type_info& rtti)
{
    MutexLock locker(sync);

    for (MetaTypeContainer::const_iterator it = metaTypes.cbegin(); it != metaTypes.cend(); ++it)
    {
        const MetaType* type = it->get();
        if (type->rtti()->hash_code() == rtti.hash_code())
        {
            return type;
        }
    }
    return nullptr;
}

const MetaType& MetaData::getMetaType(MetaType::TypeId type)
{
    MutexLock locker(sync);
    ASSERT(static_cast<size_t>(type) < metaTypes.size(), "Type not registered to be reflectable.");
    return *metaTypes[static_cast<size_t>(type)].get();
}

void MetaData::addMetaClass(const MetaClass& metaClass)
{
    std::string name = MetaType::get(metaClass.metaType()).name();

    MutexLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(name);
    ASSERT(it == metaClasses.cend(), name + " MetaClass already registered!");
    {
        metaClasses.insert(std::make_pair(name, &metaClass));
    }
}

void MetaData::removeMetaClass(const MetaClass& metaClass)
{
    std::string name = MetaType::get(metaClass.metaType()).name();

    MutexLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(name);
    if (it != metaClasses.cend())
    {
        metaClasses.erase(it);
    }
}
const MetaClass* MetaData::findMetaClass(std::string_view name)
{
    MutexLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(std::string(name));
    return it != metaClasses.cend() ? it->second : nullptr;
}

} // namespace mox
