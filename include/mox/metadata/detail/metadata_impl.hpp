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

namespace
{

template <typename T>
struct naked_type
{
    typedef typename std::remove_cv<typename std::remove_pointer<typename std::remove_reference<T>::type>::type>::type type;
};

template <typename T>
struct naked_cptype
{
    typedef std::remove_const_t<std::remove_reference_t<T>> type;
};

/**********************************************************
 * Converters
 */

template <typename From, typename To, typename Function>
struct ConverterFunctor : MetatypeConverter
{
private:
    Function m_function;

    static MetaValue convert(const MetatypeConverter& converter, const void* value)
    {
        const From* in = reinterpret_cast<const From*>(value);
        const ConverterFunctor& that = static_cast<const ConverterFunctor&>(converter);
        To out = that.m_function(*in);
        return out;
    }
public:
    explicit ConverterFunctor(Function function) :
        MetatypeConverter(&ConverterFunctor::convert),
        m_function(function)
    {
    }
};

template <typename From, typename To>
struct ConverterMethod : MetatypeConverter
{
    To (From::*m_function)() const;
    explicit ConverterMethod(To (From::*function)() const)
        : MetatypeConverter(converter)
        , m_function(function)
    {
    }

    static MetaValue converter(const MetatypeConverter& converter, const void* value)
    {
        const From* source = static_cast<const From*>(value);
        const ConverterMethod& _this = static_cast<const ConverterMethod&>(converter);
        To destination = (source->*_this.m_function)();
        return destination;
    }
};

template <typename From, typename ConverterHost>
struct EmbeddedConverter : MetatypeConverter
{
    ConverterHost host;
    explicit EmbeddedConverter()
        : MetatypeConverter(converter)
    {
    }

    static MetaValue converter(const MetatypeConverter& converter, const void* value)
    {
        const EmbeddedConverter& _this = reinterpret_cast<const EmbeddedConverter&>(converter);
        return _this.host.convert(static_cast<const From*>(value));
    }
};

} // noname

namespace metadata
{

template <typename T>
const std::type_info& remove_cv()
{
    typedef typename naked_cptype<T>::type NakedType;
    return typeid(NakedType);
}

} // namespace metadata

template <typename Type>
Metatype metaType()
{
    static_assert (!is_cstring<Type>::value, "Use std::string_view in reflections instead of cstrings");
    Metatype type = metadata::findMetatype(metadata::remove_cv<Type>());
    if (type == Metatype::Invalid)
    {
        throw type_not_registered(metadata::remove_cv<Type>());
    }
    return type;
}

template <typename Type>
const MetatypeDescriptor& metatypeDescriptor()
{
    const MetatypeDescriptor* descriptor = metadata::findMetatypeDescriptor(metadata::remove_cv<Type>());
    FATAL(descriptor, std::string("metaTypeDescriptor<>(): unregistered type ") + metadata::remove_cv<Type>().name());
    return *descriptor;
}

template <typename Type>
Metatype registerMetaType(std::string_view name)
{
    Metatype newType = metadata::findMetatype(metadata::remove_cv<Type>());
    if (newType != Metatype::Invalid)
    {
        return newType;
    }

    newType = metadata::tryRegisterMetatype(metadata::remove_cv<Type>(), std::is_enum_v<Type>, std::is_class_v<Type>, std::is_pointer_v<Type>, name);

    return newType;
}

template <typename From, typename To, typename Function>
bool registerConverter(Function function)
{
    const Metatype fromType = metaType<From>();
    const Metatype toType = metaType<To>();
    return metadata::registerConverter(std::make_unique<ConverterFunctor<From, To, Function>>(function), fromType, toType);
}

template <typename From, typename To>
bool registerConverter(To (From::*function)() const)
{
    const Metatype fromType = metaType<From>();
    const Metatype toType = metaType<To>();
    return metadata::registerConverter(std::make_unique<ConverterMethod<From, To>>(function), fromType, toType);
}

} // namespace mox

#endif // METATYPE_IMPL_H