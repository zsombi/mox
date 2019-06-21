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

#include <mox/utils/type_traits.hpp>
#include <string>
#include <mox/utils/globals.hpp>

namespace mox
{

template <typename Type>
Metatype metaType()
{
    if constexpr (is_cstring<Type>::value)
    {
        return registrar::findMetatype(typeid(char*));
    }
    else
    {
        typedef typename std::remove_cv<typename std::remove_pointer<typename std::remove_reference<Type>::type>::type>::type NakedType;
        return registrar::findMetatype(typeid(NakedType));
    }
}

template <typename Type>
const MetatypeDescriptor& metatypeDescriptor()
{
    typedef typename std::remove_pointer<typename std::remove_reference<Type>::type>::type NakedType;
    const MetatypeDescriptor* descriptor = registrar::findMetatypeDescriptor(typeid(NakedType));
    ASSERT(descriptor, std::string("metaTypeDescriptor<>(): unregistered type ") + typeid(NakedType).name());
    return *descriptor;
}

template <typename Type>
Metatype registerMetaType()
{
    typedef typename std::remove_reference<typename std::remove_pointer<Type>::type>::type NakedType;
    Metatype newType = registrar::tryRegisterMetatype(typeid(NakedType), std::is_enum<Type>(), std::is_class<NakedType>());
    if constexpr (has_static_metaclass<Type>::value)
    {
        Type::StaticMetaClass::get();
    }
    return newType;
}

} // namespace mox

#endif // METATYPE_IMPL_H
