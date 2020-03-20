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

#ifndef METATYPE_DESCRIPTOR_IMPL_HPP
#define METATYPE_DESCRIPTOR_IMPL_HPP

namespace mox
{

/******************************************************************************
 *
 */
template <typename From, typename To>
MetatypeDescriptor::Converter MetatypeDescriptor::Converter::fromExplicit()
{
    using ConverterVTable = ExplicitConverter<From, To>;
    static VTable vtable = { ConverterVTable::convert };
    return Converter(Storage(), &vtable);
}

template <typename From, typename To>
MetatypeDescriptor::Converter MetatypeDescriptor::Converter::dynamicCast()
{
    using ConverterVTable = DynamicCast<From, To>;
    static VTable vtable = { ConverterVTable::convert };
    return Converter(Storage(), &vtable);
}

template <typename From, typename To, typename Function>
MetatypeDescriptor::Converter MetatypeDescriptor::Converter::fromFunction(Function function)
{
    using ConverterVTable = FunctionConverter<From, To, Function>;
    static VTable vtable = { ConverterVTable::convert };
    return Converter(Storage(function), &vtable);
}

template <class From, class To>
MetatypeDescriptor::Converter MetatypeDescriptor::Converter::fromMethod(To (From::*method)() const)
{
    using ConverterVTable = MethodConverter<From, To>;
    static VTable vtable = { ConverterVTable::convert };
    return Converter(Storage(method), &vtable);
}

template <typename From, typename To>
MetaValue MetatypeDescriptor::Converter::ExplicitConverter<From, To>::convert(const Storage&, const void* value)
{
    To out;
    if constexpr (std::is_pointer_v<From>)
    {
        auto in = reinterpret_cast<From>(const_cast<void*>(value));
        out = reinterpret_cast<To>(in);
    }
    else
    {
        const From* in = reinterpret_cast<const From*>(value);
        if constexpr (std::is_convertible_v<From, To>)
        {
            out = static_cast<To>(*in);
        }
        else
        {
            out = reinterpret_cast<To>(*in);
        }
    }
    return out;
}


template <typename From, typename To>
MetaValue MetatypeDescriptor::Converter::DynamicCast<From, To>::convert(const Storage&, const void* value)
{
    To out;
    if constexpr (std::is_pointer_v<From>)
    {
        auto in = reinterpret_cast<From>(const_cast<void*>(value));
        out = dynamic_cast<To>(in);
    }
    else
    {
        auto in = reinterpret_cast<const From*>(value);
        out = dynamic_cast<To>(in);
    }
    return out;
}


template <typename From, typename To, typename Function>
MetaValue MetatypeDescriptor::Converter::FunctionConverter<From, To, Function>::convert(const Storage& fn, const void* value)
{
    Function function = std::any_cast<Function>(fn);
    if constexpr (std::is_pointer_v<From>)
    {
        using TFrom = typename std::remove_pointer_t<std::decay_t<From>>;
        TFrom* in = const_cast<TFrom*>(reinterpret_cast<const TFrom*>(value));
        return function(in);
    }
    else
    {
        From* in = const_cast<From*>(reinterpret_cast<const From*>(value));
        return function(*in);
    }
}

template <typename From, typename To>
MetaValue MetatypeDescriptor::Converter::MethodConverter<From, To>::convert(const Storage& storage, const void* value)
{
    using MethodType = To (From::*)() const;
    const MethodType function = std::any_cast<MethodType>(storage);

    if constexpr (std::is_pointer_v<From>)
    {
        using TFrom = typename std::remove_pointer_t<std::decay_t<From>>;
        const TFrom* source = static_cast<const TFrom*>(value);
        return (source->*function)();
    }
    else
    {
        const From* source = static_cast<const From*>(value);
        return (source->*function)();
    }
}


} // namespace mox

#endif // METATYPE_DESCRIPTOR_IMPL_HPP
