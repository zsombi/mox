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

#ifndef CALLABLE_IMPL_HPP
#define CALLABLE_IMPL_HPP

#include <mox/utils/type_traits.hpp>
#include <mox/utils/function_traits.hpp>

namespace mox
{

namespace
{

template <typename Function, std::size_t Offset>
struct MetaValueToTuple
{
    template <int Index>
    static auto convert(const Callable::ArgumentPack& arguments)
    {
        if constexpr (Index == 0)
        {
            return std::tuple<>();
        }
        else
        {
            using ArgType = typename function_traits<Function>::template argument<Index - 1>::type;
            return std::tuple_cat(convert<Index - 1>(arguments), std::make_tuple(arguments.get<ArgType>(Index - 1 + Offset)));
        }
    }
};

} // noname

template <typename... Args>
Callable::ArgumentPack::ArgumentPack(Args... arguments)
{
    std::array<Variant, sizeof... (Args)> aa = {{Variant(arguments)...}};
    insert(begin(), aa.begin(), aa.end());
}

template <typename Type>
Callable::ArgumentPack& Callable::ArgumentPack::add(const Type& value)
{
    emplace_back(Variant(value));
    return *this;
}

template <typename Type>
Callable::ArgumentPack& Callable::ArgumentPack::setInstance(Type value)
{
    insert(begin(), Variant(value));
    return *this;
}

template <typename Type>
Type Callable::ArgumentPack::get(size_t index) const
{
    if (index >= size())
    {
        throw Callable::invalid_argument();
    }
    return at(index);
}

template <typename Function>
auto Callable::ArgumentPack::toTuple() const
{
    constexpr std::size_t N = function_traits<Function>::arity;
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        return std::tuple_cat(std::make_tuple(get<typename function_traits<Function>::object*>(0)),
                              MetaValueToTuple<Function, 1>::template convert<N>(*this));
    }
    else
    {
        return MetaValueToTuple<Function, 0>::template convert<N>(*this);
    }
}


template <typename Function>
Callable::Callable(Function fn)
    : m_ret(VariantDescriptor::get<typename function_traits<Function>::return_type>())
    , m_args(function_traits<Function>::argument_descriptors())
    , m_address(::address(fn))
    , m_type(static_cast<FunctionType>(function_traits<Function>::type))
    , m_isConst(function_traits<Function>::is_const)
{
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        m_classType = metaType<typename function_traits<Function>::object>();
    }

    m_invoker = [function = std::forward<Function>(fn)](const ArgumentPack& args)
    {
        auto args_tuple = args.toTuple<Function>();

        if constexpr (std::is_void_v<typename function_traits<Function>::return_type>)
        {
            std::apply(function, args_tuple);
            return Variant();
        }
        else
        {
            auto ret = std::apply(function, args_tuple);
            return Variant(ret);
        }
    };
}


} // namespace mox

#endif // CALLABLE_IMPL_HPP
