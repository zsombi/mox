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

#include <mox/utils/function_traits.hpp>

namespace mox
{

namespace
{

template <typename Function, std::size_t Offset>
struct AnyToTuple
{
    template <int Index>
    static auto convert(Callable::Arguments& arguments)
    {
        typedef typename function_traits<Function>::template argument<Index - 1>::type ArgType;
        return std::tuple_cat(convert<Index - 1>(arguments), std::make_tuple(arguments.get<ArgType>(Index - 1 + Offset)));
    }

    template <>
    static auto convert<0>(Callable::Arguments& /*arguments*/)
    {
        return std::tuple<>();
    }
};

template <typename Function>
static auto packArguments(Callable::Arguments& arguments)
{
    constexpr std::size_t N = function_traits<Function>::arity;
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        return std::tuple_cat(std::make_tuple(arguments.get<typename function_traits<Function>::object*>(0)),
                              AnyToTuple<Function, 1>::template convert<N>(arguments));
    }
    else
    {
        return AnyToTuple<Function, 0>::template convert<N>(arguments);
    }
}

} // noname


template <typename Function>
Callable::Callable(Function fn)
    : m_ret(ArgumentDescriptor::get<typename function_traits<Function>::return_type>())
    , m_args(function_traits<Function>::argument_descriptors())
    , m_type(static_cast<FunctionType>(function_traits<Function>::type))
    , m_isConst(function_traits<Function>::is_const)
{
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        m_classType = MetaType::typeId<typename function_traits<Function>::object>();
    }

    m_invoker = [function = std::move(fn)](Arguments args) -> std::any
    {
        auto args_tuple = packArguments<Function>(args);

        if constexpr (std::is_void<typename function_traits<Function>::return_type>::value)
        {
            std::apply(function, args_tuple);
            return std::any();
        }
        else
        {
            auto ret = std::apply(function, args_tuple);
            return std::any(ret);
        }
    };
}


/******************************************************************************
 * invokes
 */
template <typename Ret, typename... Arguments>
Ret invoke(const Callable& callable, Arguments... arguments)
{
    std::array<std::any, sizeof... (Arguments)> va = {{ std::any(arguments)... }};
    Callable::Arguments vargs(va.begin(), va.end());
    std::any ret = callable.apply(vargs);
    if constexpr (!std::is_void<Ret>::value)
    {
        return std::any_cast<Ret>(ret);
    }
}

template <typename Ret, class Class, typename... Arguments>
Ret invoke(Class* instance, const Callable& callable, Arguments... arguments)
{
    std::array<std::any, sizeof... (Arguments) + 1> va = {{ std::any(instance), std::any(arguments)... }};
    Callable::Arguments vargs(va.begin(), va.end());
    std::any ret = callable.apply(vargs);
    if constexpr (!std::is_void<Ret>::value)
    {
        return std::any_cast<Ret>(ret);
    }
}

} // namespace mox

#endif // CALLABLE_IMPL_HPP
