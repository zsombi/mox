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

namespace
{

template <typename T>
struct has_metaclass
{
private:
    typedef char yes_type;
    typedef long no_type;
    template <typename U> static yes_type test(decltype(&U::getStaticMetaClass));
    template <typename U> static no_type  test(...);
public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);
};

} // noname

template <typename Type>
MetaTypeDescriptor::TypeId MetaTypeDescriptor::registerMetaType()
{
    typedef typename std::remove_reference<typename std::remove_pointer<Type>::type>::type NakedType;
    const mox::MetaTypeDescriptor& newType = MetaTypeDescriptor::newMetatype(typeid(NakedType), std::is_enum<Type>(), std::is_class<NakedType>());
    if constexpr (std::is_class_v<Type> && has_metaclass<Type>::value)
    {
        Type::getStaticMetaClass();
    }
    return newType.id();
}

template<typename Type>
const MetaTypeDescriptor& MetaTypeDescriptor::get()
{
    typedef typename std::remove_pointer<typename std::remove_reference<Type>::type>::type NakedType;
    const MetaTypeDescriptor* type = findMetaType(typeid(NakedType));
    ASSERT(type, std::string("MetaTypeDescriptor::get<>: unregistered metatype: ") + typeid(Type).name());
    return *type;
}

template <typename Type>
MetaTypeDescriptor::TypeId MetaTypeDescriptor::typeId()
{
    return get<Type>().id();
}

template <typename Type>
bool MetaTypeDescriptor::isCustomType()
{
    return get<Type>().id() >= TypeId::UserType;
}

} // namespace mox

#endif // METATYPE_IMPL_H
