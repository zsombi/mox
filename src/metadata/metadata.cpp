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
#include <mox/metadata/variant.hpp>

namespace mox
{

MetaData MetaData::globalMetaData;
MetaData *MetaData::globalMetaDataPtr = nullptr;

MetaData::MetaData()
{
    globalMetaDataPtr = this;
    TRACE("Initialize metadata")
    registerAtomicTypes(*this);

    // Register converters.
    registerConverters(*this);

    initialized = true;
    TRACE("Metadata initialized")
}

MetaData::~MetaData()
{
    TRACE("Metadata died\n")
    globalMetaDataPtr = nullptr;
}

const MetatypeDescriptor& MetaData::addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.")
    ScopeLock locker(globalMetaData);

    globalMetaData.metaTypes.emplace_back(new MetatypeDescriptor(name, int(globalMetaData.metaTypes.size()), rtti, isEnum, isClass, isPointer));
    return *globalMetaData.metaTypes.back().get();
}

namespace metadata
{

MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti)
{
    if (!MetaData::globalMetaDataPtr)
    {
        std::cerr << "Warning: metatype lookup attempt after mox backend went down.\n";
        return nullptr;
    }
    ScopeLock locker(MetaData::globalMetaData);

    for (auto& type : MetaData::globalMetaData.metaTypes)
    {
        if (type->rtti()->hash_code() == rtti.hash_code())
        {
            return type.get();
        }
    }
    // Find synonym types.
    for (auto& synonym : MetaData::globalMetaData.synonymTypes)
    {
        if (synonym.first->hash_code() == rtti.hash_code())
        {
            return MetaData::globalMetaData.metaTypes[static_cast<size_t>(synonym.second)].get();
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
    const MetatypeDescriptor* type = findMetatypeDescriptor(rtti);
    if (!type)
    {
        const MetatypeDescriptor& newType = MetaData::addMetaType(name.data(), rtti, isEnum, isClass, isPointer);
        type = &newType;
    }
    return type->id();
}

bool registerConverter(MetatypeConverterPtr&& converter, Metatype fromType, Metatype toType)
{
    MetatypeDescriptor& descriptor = MetaData::getMetaType(fromType);
    return descriptor.addConverter(std::forward<MetatypeConverterPtr>(converter), toType);
}

MetatypeConverter* findConverter(Metatype from, Metatype to)
{
    MetatypeDescriptor& descriptor = MetaData::getMetaType(from);
    return descriptor.findConverterTo(to);
}

} // namespace metadata

MetatypeDescriptor& MetaData::getMetaType(Metatype type)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.")
    ScopeLock locker(globalMetaData);
    FATAL(static_cast<size_t>(type) < globalMetaData.metaTypes.size(), "Type not registered to be reflectable.");
    return *globalMetaData.metaTypes[static_cast<size_t>(type)].get();
}

void MetaData::addMetaClass(const MetaClass& metaClass)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.")
    std::string name = MetatypeDescriptor::get(metaClass.metaType()).name();

    ScopeLock locker(globalMetaData);
    MetaClassContainer::const_iterator it = globalMetaData.metaClasses.find(name);
    FATAL(it == globalMetaData.metaClasses.cend(), name + " MetaClass already registered!");

    globalMetaData.metaClassRegister.insert(std::make_pair(metaClass.metaType(), &metaClass));
    globalMetaData.metaClasses.insert(std::make_pair(name, &metaClass));

    TRACE("MetaClass added: " << name)
}

void MetaData::removeMetaClass(const MetaClass& metaClass)
{
    if (!globalMetaDataPtr)
    {
        std::cerr << "Warning: MetaClass removal attempt after mox backend went down.\n";
        return;
    }
    std::string name = MetatypeDescriptor::get(metaClass.metaType()).name();

    ScopeLock locker(globalMetaData);
    MetaClassContainer::const_iterator it = globalMetaData.metaClasses.find(name);
    if (it != globalMetaData.metaClasses.cend())
    {
        globalMetaData.metaClasses.erase(it);
    }
    MetaClassTypeRegister::const_iterator cit = globalMetaData.metaClassRegister.find(metaClass.metaType());
    if (cit != globalMetaData.metaClassRegister.cend())
    {
        globalMetaData.metaClassRegister.erase(cit);
    }

    TRACE("MetaClass " << name << " removed")
}

const MetaClass* MetaData::findMetaClass(std::string_view name)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.")
    ScopeLock locker(globalMetaData);
    MetaClassContainer::const_iterator it = globalMetaData.metaClasses.find(std::string(name));
    return it != globalMetaData.metaClasses.cend() ? it->second : nullptr;
}

const MetaClass* MetaData::getMetaClass(Metatype metaType)
{
    FATAL(globalMetaDataPtr, "mox is not initialized or down.")
    ScopeLock locker(globalMetaData);
    MetaClassTypeRegister::const_iterator it = globalMetaData.metaClassRegister.find(metaType);
    return it != globalMetaData.metaClassRegister.cend() ? it->second : nullptr;
}

} // namespace mox
