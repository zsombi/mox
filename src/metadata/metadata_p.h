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
#include <mox/metadata/metaclass.hpp>

namespace mox
{

typedef std::pair<MetaTypeDescriptor::TypeId, MetaTypeDescriptor::TypeId> ConverterKeyType;

} // namespace mox

namespace std
{

template<>
struct hash<mox::ConverterKeyType>
{
    std::size_t operator()(const mox::ConverterKeyType& arg) const
    {
        return hash<int>()(int(arg.first)) ^ (hash<int>()(int(arg.second)) << 1);
    }
};

} // namespace std

namespace mox {

class MetaData
{
public:
    explicit MetaData();
    ~MetaData();
    void initialize();

    const MetaTypeDescriptor& addMetaType(const char* name, const std::type_info& rtti, bool isEnum, bool isClass);
    const MetaTypeDescriptor* findMetaType(const std::type_info& rtti);
    const MetaTypeDescriptor& getMetaType(MetaTypeDescriptor::TypeId type);

    void addMetaClass(const MetaClass& metaClass);
    void removeMetaClass(const MetaClass& metaClass);
    const MetaClass* findMetaClass(std::string_view name);
    const MetaClass* getMetaClass(MetaTypeDescriptor::TypeId metaType);

    bool addConverter(MetaTypeDescriptor::AbstractConverterSharedPtr converter, MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType);
    void removeConverter(MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType);
    MetaTypeDescriptor::AbstractConverterSharedPtr findConverter(MetaTypeDescriptor::TypeId fromType, MetaTypeDescriptor::TypeId toType);

    typedef std::vector<std::unique_ptr<MetaTypeDescriptor>> MetaTypeContainer;
    typedef std::unordered_map<MetaTypeDescriptor::TypeId, const MetaClass*> MetaClassTypeRegister;
    typedef std::unordered_map<std::string, const MetaClass*> MetaClassContainer;
    typedef std::unordered_map<ConverterKeyType, MetaTypeDescriptor::AbstractConverterSharedPtr> ConverterContainer;

    MetaTypeContainer metaTypes;
    ConverterContainer converters;
    MetaClassTypeRegister metaClassRegister;
    MetaClassContainer metaClasses;
    std::mutex sync;
};

MetaData& metadata();

} // namespace mox

#endif // METADATA_P_H
