/*
 * Copyright (C) 2017-2019 bitWelder
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
#include <mox/metadata/metaobject.hpp>

namespace mox
{

MetaObject::MetaObject()
{
}

MetaObject::~MetaObject()
{
}

MetaObject::StaticMetaClass::StaticMetaClass()
    : MetaClass(*MetaData::globalMetaData.findMetaType(typeid(MetaObject)))
{
}

const MetaObject::StaticMetaClass* MetaObject::StaticMetaClass::get()
{
    static MetaObject::StaticMetaClass staticMetaClass;
    return &staticMetaClass;
}

bool MetaObject::StaticMetaClass::isAbstract() const
{
    return false;
}

bool MetaObject::StaticMetaClass::isClassOf(const MetaObject &) const
{
    return true;
}

} // namespace mox
