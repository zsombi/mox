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

#include <mox/metadata/metaclass.hpp>

using namespace mox;

#undef METACLASS
#define METACLASS(thisClass, ...) \
    static const mox::MetaClass* getStaticMetaClass() \
    { \
        static StaticMetaClass metaClass; \
        return &metaClass; \
    } \
    struct MOX_API StaticMetaClass : mox::MetaClassImpl<thisClass> \


class BaseClass
{
public:
    METACLASS(BaseClass)
//    static const mox::MetaClass* getStaticMetaClass()
//    {
//        static StaticMetaClass metaClass;
//        return &metaClass;
//    }
//    struct MOX_API StaticMetaClass : mox::MetaClassImpl<BaseClass>
    {
    };

    explicit BaseClass()
    {
    }
};
