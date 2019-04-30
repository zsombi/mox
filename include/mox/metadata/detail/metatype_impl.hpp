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

#ifndef METATYPE_IMPL_H
#define METATYPE_IMPL_H

#include <type_traits>

namespace mox
{

template <typename Type>
const MetaType& MetaType::registerMetaType()
{
    typedef typename std::remove_reference<typename std::remove_pointer<Type>::type>::type NakedType;
    return MetaType::newMetatype(typeid(NakedType), std::is_enum<Type>(), std::is_class<NakedType>());
}

template<typename Type>
const MetaType& MetaType::get()
{
    typedef typename std::remove_pointer<typename std::remove_reference<Type>::type>::type NakedType;
    const MetaType* type = findMetaType(typeid(NakedType));
    ASSERT(type, std::string("MetaType::get<>: unregistered metatype: ") + typeid(Type).name());
    return *type;
}

template <typename Type>
MetaType::TypeId MetaType::typeId()
{
    return get<Type>().id();
}

template <typename Type>
bool MetaType::isCustomType()
{
    return get<Type>().id() >= TypeId::UserType;
}

} // namespace mox

#endif // METATYPE_IMPL_H
