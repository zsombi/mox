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
    MetaType::registerConverter<From, To>(atomicConverter<From, To>);
    MetaType::registerConverter<To, From>(atomicConverter<To, From>);
}

template <typename From, typename To>
void registerPointerConverters()
{
    MetaType::registerConverter<From, To>(pointerConverter<From, To>);
    MetaType::registerConverter<To, From>(pointerConverter<To, From>);
}

void registerStringConverters()
{
    MetaType::registerConverter<bool, std::string>(default_converters::boolToString);
    MetaType::registerConverter<std::string, bool>(default_converters::stringToBool);

    MetaType::registerConverter<char, std::string>([](char v) { std::string s; s += v; return s; });
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

    ATOMIC_TYPE("void", void, MetaType::TypeId::Void);
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
    ATOMIC_TYPE("std::string", std::string, MetaType::TypeId::String);
    ATOMIC_TYPE("MetaObject", MetaObject, MetaType::TypeId::MetaObject);
    ATOMIC_TYPE("void*", void*, MetaType::TypeId::VoidPtr);

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

bool MetaData::addConverter(MetaType::AbstractConverterSharedPtr converter, MetaType::TypeId fromType, MetaType::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    if (converters.find(key) != converters.end())
    {
        return false;
    }
    return converters.insert(std::make_pair(key, converter)).second;
}

void MetaData::removeConverter(MetaType::TypeId fromType, MetaType::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    ConverterContainer::const_iterator it = converters.find(key);
    if (it != converters.end())
    {
        converters.erase(it);
    }
}

MetaType::AbstractConverterSharedPtr MetaData::findConverter(MetaType::TypeId fromType, MetaType::TypeId toType)
{
    ConverterKeyType key = std::make_pair(fromType, toType);
    ConverterContainer::iterator it = converters.find(key);
    return (it != converters.end()) ? it->second : nullptr;
}

} // namespace mox
