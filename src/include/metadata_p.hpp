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

#ifndef METADATA_P_H
#define METADATA_P_H

#include <vector>
#include <string>
#include <functional>
#include <mox/config/deftypes.hpp>
#include <mox/utils/containers/flat_map.hpp>
#include <mox/metatype.core/metadata.hpp>
#include <mox/metatype.core/metatype_descriptor.hpp>
#include <mox/metatype.core/variant.hpp>
#include <mox/metainfo/metaclass.hpp>
#include <mox/metainfo/metaobject.hpp>

namespace mox
{

struct MetaData : public MetaBase
{
    explicit MetaData();
    ~MetaData();

    static const MetatypeDescriptor& addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer);
    static MetatypeDescriptor& getMetaType(Metatype type);

    static void addMetaClass(const metainfo::MetaClass& metaClass);
    static void removeMetaClass(const metainfo::MetaClass& metaClass);
    static const metainfo::MetaClass* findMetaClass(std::string_view name);
    static const metainfo::MetaClass* getMetaClass(Metatype metaType);

    typedef std::vector<std::unique_ptr<MetatypeDescriptor>> MetaTypeContainer;
    typedef std::vector<std::pair<const std::type_info*, Metatype>> SynonymContainer;
    typedef FlatMap<Metatype, const metainfo::MetaClass*> MetaClassTypeRegister;
    typedef FlatMap<std::string, const metainfo::MetaClass*> MetaClassContainer;

    MetaTypeContainer metaTypes;
    SynonymContainer synonymTypes;
    MetaClassTypeRegister metaClassRegister;
    MetaClassContainer metaClasses;
    bool initialized = false;

    static MetaData globalMetaData;
    static MetaData* globalMetaDataPtr;
};

void registerAtomicTypes(MetaData& metaData);
void registerConverters();

TUuid nextUuid();

} // namespace mox

#endif // METADATA_P_H
