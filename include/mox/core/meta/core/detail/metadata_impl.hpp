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
#include <mox/config/error.hpp>
#include <mox/core/meta/core/metatype_descriptor.hpp>

namespace mox
{

template <typename Type>
Metatype metaType()
{
    static_assert (!is_cstring<Type>::value, "Use std::string_view in reflections instead of cstrings");
    Metatype type = metadata::findMetatype(getNakedTypeInfo<Type>());
    throwIf<ExceptionType::MetatypeNotRegistered>(type == Metatype::Invalid);
    return type;
}

template <typename Type>
const MetatypeDescriptor& metatypeDescriptor()
{
    const MetatypeDescriptor* descriptor = metadata::findMetatypeDescriptor(getNakedTypeInfo<Type>());
    FATAL(descriptor, std::string("metaTypeDescriptor<>(): unregistered type ") + getNakedTypeInfo<Type>().name());
    return *descriptor;
}

template <typename Type>
Metatype registerMetaType(std::string_view name)
{
    const auto& rtti = getNakedTypeInfo<Type>();
    auto newType = metadata::findMetatype(rtti);
    if (newType != Metatype::Invalid)
    {
        return newType;
    }

    newType = metadata::tryRegisterMetatype(rtti, std::is_enum_v<Type>, std::is_class_v<Type>, std::is_pointer_v<Type>, name);
    if constexpr (std::is_pointer_v<Type>)
    {
        mox::registerConverter<Type, intptr_t>();
        mox::registerConverter<intptr_t, Type>();
    }

    return newType;
}

template <class Class>
std::pair<Metatype, Metatype> registerClassMetaTypes(std::string_view name)
{
    using PtrClass = std::add_pointer_t<Class>;
    auto staticType = registerMetaType<Class>(name);
    auto pointerType = registerMetaType<PtrClass>(name.empty() ? "" : std::string(name) + "*");
    return std::make_pair(staticType, pointerType);
}


template <typename From, typename To>
bool registerConverter()
{
    const Metatype toType = metaType<To>();

    MetatypeDescriptor* descriptor = metadata::findMetatypeDescriptor(getNakedTypeInfo<From>());
    if constexpr(std::is_class_v<typename std::remove_pointer_t<From>> && std::is_class_v<typename std::remove_pointer_t<To>>)
    {
        return descriptor->addConverter(MetatypeDescriptor::Converter::dynamicCast<From, To>(), toType);
    }
    else
    {
        return descriptor->addConverter(MetatypeDescriptor::Converter::fromExplicit<From, To>(), toType);
    }
}

template <typename From, typename To, typename Function>
bool registerConverter(Function function)
{
    const Metatype toType = metaType<To>();
    MetatypeDescriptor* descriptor = metadata::findMetatypeDescriptor(getNakedTypeInfo<From>());
    return descriptor->addConverter(MetatypeDescriptor::Converter::fromFunction<From, To, Function>(function), toType);
}

template <typename From, typename To>
bool registerConverter(To (From::*method)() const)
{
    const Metatype toType = metaType<To>();
    MetatypeDescriptor* descriptor = metadata::findMetatypeDescriptor(getNakedTypeInfo<From>());
    return descriptor->addConverter(MetatypeDescriptor::Converter::fromMethod<From, To>(method), toType);
}

} // namespace mox

#endif // METATYPE_IMPL_H
