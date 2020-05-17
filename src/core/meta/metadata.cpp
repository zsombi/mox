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

#include <metadata_p.hpp>
#include <mox/core/meta/core/metadata.hpp>
#include <mox/core/meta/core/metatype_descriptor.hpp>
#include <mox/core/meta/class/metaclass.hpp>
#include <mox/utils/locks.hpp>
#include <mox/config/string.hpp>
#include <mox/core/meta/core/variant.hpp>
#include <algorithm>

#ifdef MOX_ENABLE_LOGS
#include <mox/utils/log/logger.hpp>
#endif

namespace mox
{

MetaData::MetaData()
{
#ifdef MOX_ENABLE_LOGS
    LoggerData::get();
#endif

    FATAL(!globalMetaDataPtr, "global metadata store initialized twice!");
    globalMetaDataPtr = this;
    CTRACE(metacore, "Initialize metadata");
}

MetaData::~MetaData()
{
    globalMetaDataPtr = nullptr;
}

const MetatypeDescriptor& MetaData::addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.");
    lock_guard locker(*globalMetaDataPtr);

    globalMetaDataPtr->metaTypes.emplace_back(new MetatypeDescriptor(name, int(globalMetaDataPtr->metaTypes.size()), rtti, isEnum, isClass, isPointer));
    return *globalMetaDataPtr->metaTypes.back().get();
}

namespace metadata
{

const MetatypeDescriptor* findMetatype(const std::function<bool(const MetatypeDescriptor&)>& predicate)
{
    if (!MetaData::globalMetaDataPtr)
    {
        CWARN(metacore, "metatype lookup attempt after mox backend went down.");
        return nullptr;
    }
    lock_guard locker(*MetaData::globalMetaDataPtr);

    for (auto& type : MetaData::globalMetaDataPtr->metaTypes)
    {
        if (predicate(*type))
        {
            return type.get();
        }
    }
    return nullptr;
}

MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti)
{
    if (!MetaData::globalMetaDataPtr)
    {
        CWARN(metacore, "metatype lookup attempt after mox backend went down.");
        return nullptr;
    }
    lock_guard locker(*MetaData::globalMetaDataPtr);

    for (auto& type : MetaData::globalMetaDataPtr->metaTypes)
    {
        if (type->rtti()->hash_code() == rtti.hash_code())
        {
            return type.get();
        }
    }
    // Find synonym types.
    for (auto& synonym : MetaData::globalMetaDataPtr->synonymTypes)
    {
        if (synonym.first->hash_code() == rtti.hash_code())
        {
            return MetaData::globalMetaDataPtr->metaTypes[static_cast<size_t>(synonym.second)].get();
        }
    }
    return nullptr;
}

Metatype findMetatype(const std::type_info& rtti)
{
    const MetatypeDescriptor* descriptor = findMetatypeDescriptor(rtti);
    return descriptor ? descriptor->id() : Metatype::Invalid;
}

Metatype tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass, bool isPointer, std::string_view name)
{
    static MetaData metadata;
    if (!metadata.initialized)
    {
        metadata.initialized = true;
        metadata.registerAtomicTypes();
        metadata.registerConverters();
    }

    const MetatypeDescriptor* type = findMetatypeDescriptor(rtti);
    if (!type)
    {
        const MetatypeDescriptor& newType = MetaData::addMetaType(name.data(), rtti, isEnum, isClass, isPointer);
        type = &newType;
    }
    return type->id();
}

} // namespace metadata

namespace metainfo
{

const MetaClass* find(std::function<bool(const MetaClass&)> predicate)
{
    if (!MetaData::globalMetaDataPtr)
    {
        CWARN(metacore, "metaclass lookup attempt after mox backend went down.");
        return nullptr;
    }
    lock_guard locker(*MetaData::globalMetaDataPtr);

    for (auto& metaclass : MetaData::globalMetaDataPtr->metaClasses)
    {
        ScopeRelock relock(*MetaData::globalMetaDataPtr);
        if (predicate(*metaclass.second))
        {
            return metaclass.second;
        }
    }
    return nullptr;
}

} // metainfo

MetatypeDescriptor& MetaData::getMetaType(Metatype type)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.");
    lock_guard locker(*globalMetaDataPtr);
    FATAL(static_cast<size_t>(type) < globalMetaDataPtr->metaTypes.size(), "Type not registered to be reflectable.");
    return *globalMetaDataPtr->metaTypes[static_cast<size_t>(type)].get();
}

void MetaData::addMetaClass(const metainfo::MetaClass& metaClass)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.");
    std::string name = MetatypeDescriptor::get(metaClass.getMetaTypes().first).name();

    lock_guard locker(*globalMetaDataPtr);
    auto it = globalMetaDataPtr->metaClasses.find(name);
    FATAL(it == globalMetaDataPtr->metaClasses.cend(), "Static metaclass for '" + name + "' already registered!");

    globalMetaDataPtr->metaClassRegister.insert({metaClass.getMetaTypes().first, &metaClass});
    globalMetaDataPtr->metaClasses.insert({name, &metaClass});

    CTRACE(metacore, "MetaClass added:" << name);
}

void MetaData::removeMetaClass(const metainfo::MetaClass& metaClass)
{
    if (!globalMetaDataPtr)
    {
        CWARN(metacore, "MetaClass removal attempt after mox backend went down.");
        return;
    }
    std::string name = MetatypeDescriptor::get(metaClass.getMetaTypes().first).name();

    lock_guard locker(*globalMetaDataPtr);
    auto it = globalMetaDataPtr->metaClasses.find(name);
    if (it != globalMetaDataPtr->metaClasses.cend())
    {
        globalMetaDataPtr->metaClasses.erase(it, it);
    }
    auto cit = globalMetaDataPtr->metaClassRegister.find(metaClass.getMetaTypes().first);
    if (cit != globalMetaDataPtr->metaClassRegister.cend())
    {
        globalMetaDataPtr->metaClassRegister.erase(cit, cit);
    }

    CTRACE(metacore, "MetaClass" << name << "removed");
}

const metainfo::MetaClass* MetaData::findMetaClass(std::string_view name)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.");
    lock_guard locker(*globalMetaDataPtr);
    auto it = globalMetaDataPtr->metaClasses.find(std::string(name));
    return it != globalMetaDataPtr->metaClasses.cend() ? it->second : nullptr;
}

const metainfo::MetaClass* MetaData::getMetaClass(Metatype metaType)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.");
    lock_guard locker(*globalMetaDataPtr);
    auto it = globalMetaDataPtr->metaClassRegister.find(metaType);
    return it != globalMetaDataPtr->metaClassRegister.cend() ? it->second : nullptr;
}

} // namespace mox
