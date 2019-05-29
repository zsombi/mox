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
#include <mox/utils/string.hpp>
#include <mox/metadata/variant.hpp>

namespace mox
{

namespace default_converters
{

template <typename From, typename To>
To atomicConverter(From value)
{
    return static_cast<To>(value);
}

template <typename From, typename To>
To pointerConverter(From value)
{
    return reinterpret_cast<To>(value);
}

std::string boolToString(bool value)
{
    return value ? "true" : "false";
}

bool stringToBool(std::string value)
{
    return (std::string_tolower(value) == "true");
}

template <typename From, typename To>
void registerTypeConverters()
{
    MetaTypeDescriptor::registerConverter<From, To>(atomicConverter<From, To>);
    MetaTypeDescriptor::registerConverter<To, From>(atomicConverter<To, From>);
}

template <typename From, typename To>
void registerPointerConverters()
{
    MetaTypeDescriptor::registerConverter<From, To>(pointerConverter<From, To>);
    MetaTypeDescriptor::registerConverter<To, From>(pointerConverter<To, From>);
}

void registerStringConverters()
{
    MetaTypeDescriptor::registerConverter<bool, std::string>(default_converters::boolToString);
    MetaTypeDescriptor::registerConverter<std::string, bool>(default_converters::stringToBool);

    MetaTypeDescriptor::registerConverter<char, std::string>([](char v) { std::string s; s += v; return s; });
}

} // namespace default_converters

MetaData::MetaData()
{
}

MetaData::~MetaData()
{
}

#define ATOMIC_TYPE(name, Type, typeId) \
{ \
    const MetaTypeDescriptor& metaType = addMetaType(name, typeid(Type), std::is_enum<Type>(), std::is_class<Type>()); \
    ASSERT(metaType.id() == typeId, "wrong atomic type registration!"); \
}

void MetaData::initialize()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }

    ATOMIC_TYPE("void", void, MetaTypeDescriptor::TypeId::Void);
    ATOMIC_TYPE("bool", bool, MetaTypeDescriptor::TypeId::Bool);
    ATOMIC_TYPE("char", char, MetaTypeDescriptor::TypeId::Char);
    ATOMIC_TYPE("byte", byte, MetaTypeDescriptor::TypeId::Byte);
    ATOMIC_TYPE("short", short, MetaTypeDescriptor::TypeId::Short);
    ATOMIC_TYPE("word", unsigned short, MetaTypeDescriptor::TypeId::Word);
    ATOMIC_TYPE("int", int, MetaTypeDescriptor::TypeId::Int);
    ATOMIC_TYPE("uint", unsigned int, MetaTypeDescriptor::TypeId::UInt);
    ATOMIC_TYPE("long", long, MetaTypeDescriptor::TypeId::Long);
    ATOMIC_TYPE("ulong", unsigned long, MetaTypeDescriptor::TypeId::ULong);
    ATOMIC_TYPE("int64", long long, MetaTypeDescriptor::TypeId::Int64);
    ATOMIC_TYPE("uint64", unsigned long long, MetaTypeDescriptor::TypeId::UInt64);
    ATOMIC_TYPE("float", float, MetaTypeDescriptor::TypeId::Float);
    ATOMIC_TYPE("double", double, MetaTypeDescriptor::TypeId::Double);
    ATOMIC_TYPE("std::string", std::string, MetaTypeDescriptor::TypeId::String);
    ATOMIC_TYPE("MetaObject", MetaObject, MetaTypeDescriptor::TypeId::MetaObject);
    ATOMIC_TYPE("void*", void*, MetaTypeDescriptor::TypeId::VoidPtr);

    // Mark atomic type initialization completed.
    initialized = true;

    // Register converters.
    default_converters::registerStringConverters();
    default_converters::registerTypeConverters<bool, char>();
    default_converters::registerTypeConverters<bool, byte>();
    default_converters::registerTypeConverters<bool, short>();
    default_converters::registerTypeConverters<bool, unsigned short>();
    default_converters::registerTypeConverters<bool, int>();
    default_converters::registerTypeConverters<bool, unsigned int>();
    default_converters::registerTypeConverters<bool, long>();
    default_converters::registerTypeConverters<bool, unsigned long>();
    default_converters::registerTypeConverters<bool, long long>();
    default_converters::registerTypeConverters<bool, unsigned long long>();
    default_converters::registerTypeConverters<bool, float>();
    default_converters::registerTypeConverters<bool, double>();

    default_converters::registerTypeConverters<char, byte>();
    default_converters::registerTypeConverters<char, short>();
    default_converters::registerTypeConverters<char, unsigned short>();
    default_converters::registerTypeConverters<char, int>();
    default_converters::registerTypeConverters<char, unsigned int>();
    default_converters::registerTypeConverters<char, long>();
    default_converters::registerTypeConverters<char, unsigned long>();
    default_converters::registerTypeConverters<char, long long>();
    default_converters::registerTypeConverters<char, unsigned long long>();
    default_converters::registerTypeConverters<char, float>();
    default_converters::registerTypeConverters<char, double>();

    default_converters::registerTypeConverters<byte, short>();
    default_converters::registerTypeConverters<byte, unsigned short>();
    default_converters::registerTypeConverters<byte, int>();
    default_converters::registerTypeConverters<byte, unsigned int>();
    default_converters::registerTypeConverters<byte, long>();
    default_converters::registerTypeConverters<byte, unsigned long>();
    default_converters::registerTypeConverters<byte, long long>();
    default_converters::registerTypeConverters<byte, unsigned long long>();
    default_converters::registerTypeConverters<byte, float>();
    default_converters::registerTypeConverters<byte, double>();

    default_converters::registerTypeConverters<short, unsigned short>();
    default_converters::registerTypeConverters<short, int>();
    default_converters::registerTypeConverters<short, unsigned int>();
    default_converters::registerTypeConverters<short, long>();
    default_converters::registerTypeConverters<short, unsigned long>();
    default_converters::registerTypeConverters<short, long long>();
    default_converters::registerTypeConverters<short, unsigned long long>();
    default_converters::registerTypeConverters<short, float>();
    default_converters::registerTypeConverters<short, double>();

    default_converters::registerTypeConverters<unsigned short, int>();
    default_converters::registerTypeConverters<unsigned short, unsigned int>();
    default_converters::registerTypeConverters<unsigned short, long>();
    default_converters::registerTypeConverters<unsigned short, unsigned long>();
    default_converters::registerTypeConverters<unsigned short, long long>();
    default_converters::registerTypeConverters<unsigned short, unsigned long long>();
    default_converters::registerTypeConverters<unsigned short, float>();
    default_converters::registerTypeConverters<unsigned short, double>();

    default_converters::registerTypeConverters<int, unsigned int>();
    default_converters::registerTypeConverters<int, long>();
    default_converters::registerTypeConverters<int, unsigned long>();
    default_converters::registerTypeConverters<int, long long>();
    default_converters::registerTypeConverters<int, unsigned long long>();
    default_converters::registerTypeConverters<int, float>();
    default_converters::registerTypeConverters<int, double>();
    default_converters::registerTypeConverters<int, byte>();

    default_converters::registerTypeConverters<unsigned int, long>();
    default_converters::registerTypeConverters<unsigned int, unsigned long>();
    default_converters::registerTypeConverters<unsigned int, long long>();
    default_converters::registerTypeConverters<unsigned int, unsigned long long>();
    default_converters::registerTypeConverters<unsigned int, float>();
    default_converters::registerTypeConverters<unsigned int, double>();

    default_converters::registerTypeConverters<long, unsigned long>();
    default_converters::registerTypeConverters<long, long long>();
    default_converters::registerTypeConverters<long, unsigned long long>();
    default_converters::registerTypeConverters<long, float>();
    default_converters::registerTypeConverters<long, double>();

    default_converters::registerTypeConverters<unsigned long, long long>();
    default_converters::registerTypeConverters<unsigned long, unsigned long long>();
    default_converters::registerTypeConverters<unsigned long, float>();
    default_converters::registerTypeConverters<unsigned long, double>();

    default_converters::registerTypeConverters<long long, unsigned long long>();
    default_converters::registerTypeConverters<long long, float>();
    default_converters::registerTypeConverters<long long, double>();

    default_converters::registerTypeConverters<unsigned long long, float>();
    default_converters::registerTypeConverters<unsigned long long, double>();

    default_converters::registerTypeConverters<float, double>();

    default_converters::registerPointerConverters<unsigned long long, void*>();
    default_converters::registerPointerConverters<unsigned long long, MetaObject*>();
    default_converters::registerPointerConverters<void*, MetaObject*>();
}

MetaData& metadata()
{
    static MetaData globalMetaData;
    globalMetaData.initialize();
    return globalMetaData;
}

const MetaTypeDescriptor& MetaData::addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass)
{
    MutexLock locker(sync);

    metaTypes.emplace_back(new MetaTypeDescriptor(name, int(metaTypes.size()), rtti, isEnum, isClass));
    return *metaTypes.back().get();
}

const MetaTypeDescriptor* MetaData::findMetaType(const std::type_info& rtti)
{
    MutexLock locker(sync);

    for (MetaTypeContainer::const_iterator it = metaTypes.cbegin(); it != metaTypes.cend(); ++it)
    {
        const MetaTypeDescriptor* type = it->get();
        if (type->rtti()->hash_code() == rtti.hash_code())
        {
            return type;
        }
    }
    return nullptr;
}

const MetaTypeDescriptor& MetaData::getMetaType(MetaTypeDescriptor::TypeId type)
{
    MutexLock locker(sync);
    ASSERT(static_cast<size_t>(type) < metaTypes.size(), "Type not registered to be reflectable.");
    return *metaTypes[static_cast<size_t>(type)].get();
}

void MetaData::addMetaClass(const MetaClass& metaClass)
{
    std::string name = MetaTypeDescriptor::get(metaClass.metaType()).name();

    MutexLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(name);
    ASSERT(it == metaClasses.cend(), name + " MetaClass already registered!");

    metaClassRegister.insert(std::make_pair(metaClass.metaType(), &metaClass));
    metaClasses.insert(std::make_pair(name, &metaClass));
}

void MetaData::removeMetaClass(const MetaClass& metaClass)
{
    std::string name = MetaTypeDescriptor::get(metaClass.metaType()).name();

    MutexLock locker(sync);
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
    MutexLock locker(sync);
    MetaClassContainer::const_iterator it = metaClasses.find(std::string(name));
    return it != metaClasses.cend() ? it->second : nullptr;
}

const MetaClass* MetaData::getMetaClass(MetaTypeDescriptor::TypeId metaType)
{
    MutexLock locker(sync);
    MetaClassTypeRegister::const_iterator it = metaClassRegister.find(metaType);
    return it != metaClassRegister.cend() ? it->second : nullptr;
}

bool MetaData::addConverter(MetaTypeDescriptor::AbstractConverterSharedPtr converter, MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    if (converters.find(key) != converters.end())
    {
        return false;
    }
    return converters.insert(std::make_pair(key, converter)).second;
}

void MetaData::removeConverter(MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    ConverterContainer::const_iterator it = converters.find(key);
    if (it != converters.end())
    {
        converters.erase(it);
    }
}

MetaTypeDescriptor::AbstractConverterSharedPtr MetaData::findConverter(MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    ConverterContainer::iterator it = converters.find(key);
    return (it != converters.end()) ? it->second : nullptr;
}

} // namespace mox
