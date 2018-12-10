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
#include <mox/metadata/metatype.hpp>

namespace mox {

class MetaData
{
public:
    explicit MetaData();
    ~MetaData();
    void initialize();

    const MetaType& addMetaType(const char* name, const std::type_info& rtti, bool isEnum);
    const MetaType* findMetaType(const std::type_info& rtti);
    const MetaType& getMetaType(MetaType::TypeId type);

    typedef std::vector<std::unique_ptr<MetaType>> MetaTypeContainer;

    MetaTypeContainer metaTypes;
    std::mutex sync;
};

MetaData& metadata();

} // namespace mox

#endif // METADATA_P_H
