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

#ifndef METACLASS_HPP
#define METACLASS_HPP

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox
{

struct MOX_API MetaClass
{
public:
    explicit MetaClass(MetaType::TypeId type);
    virtual ~MetaClass();

protected:
    typedef std::vector<MetaClass*> SuperClassContainer;

    SuperClassContainer m_superClasses;
    MetaType::TypeId m_type;
};


template <class ThisClass, class... SuperClasses>
struct MetaClassImpl : MetaClass
{
public:
    explicit MetaClassImpl()
        : MetaClass(MetaType::typeId<ThisClass>())
    {
        std::array<MetaClass*, sizeof... (SuperClasses)> aa =
        {{
            SuperClasses::getStaticMetaClass()...
        }};
        m_superClasses = SuperClassContainer(aa.begin(), aa.end());
    }
};

} // namespace mox

#define METACLASS(thisClass, ...) \
    static const mox::MetaClass* getStaticMetaClass() \
    { \
        static StaticMetaClass metaClass; \
        return &metaClass; \
    } \
    struct MOX_API StaticMetaClass : mox::MetaClassImpl<thisClass, __VA_ARGS__>


#endif // METACLASS_HPP
