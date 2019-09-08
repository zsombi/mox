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
#include <unordered_map>
#include <string>
#include <functional>
#include <mox/metadata/metatype.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <mox/metadata/argument.hpp>
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metaobject.hpp>

namespace mox
{

class MetaData
{
public:
    explicit MetaData();
    ~MetaData();
    void initialize();

    const MetatypeDescriptor& addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer);
    const MetatypeDescriptor* findMetaType(const std::type_info& rtti);
    MetatypeDescriptor& getMetaType(Metatype type);

    void addMetaClass(const MetaClass& metaClass);
    void removeMetaClass(const MetaClass& metaClass);
    const MetaClass* findMetaClass(std::string_view name);
    const MetaClass* getMetaClass(Metatype metaType);

    typedef std::vector<std::unique_ptr<MetatypeDescriptor>> MetaTypeContainer;
    typedef std::vector<std::pair<const std::type_info*, Metatype>> SynonymContainer;
    typedef std::unordered_map<Metatype, const MetaClass*> MetaClassTypeRegister;
    typedef std::unordered_map<std::string, const MetaClass*> MetaClassContainer;

    MetaTypeContainer metaTypes;
    SynonymContainer synonymTypes;
    MetaClassTypeRegister metaClassRegister;
    MetaClassContainer metaClasses;
    std::mutex sync;
};

MetaData& metadata();

void registerAtomicTypes();
void registerConverters();

} // namespace mox

#endif // METADATA_P_H
