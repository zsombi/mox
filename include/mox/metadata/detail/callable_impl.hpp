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
struct ArgumentsToTuple
{
    template <int Index>
    static auto convert(const Callable::Arguments& arguments)
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
Callable::Arguments::Arguments(Args... arguments)
{
    std::array<Argument, sizeof... (Args)> aa = {{Argument(arguments)...}};
    insert(begin(), aa.begin(), aa.end());
}

template <typename Type>
Callable::Arguments& Callable::Arguments::add(const Type& value)
{
    emplace_back(Argument(value));
    return *this;
}

template <typename Type>
Callable::Arguments& Callable::Arguments::setInstance(Type value)
{
    insert(begin(), Argument(value));
    return *this;
}

template <typename Type>
Type Callable::Arguments::get(size_t index) const
{
    if (index >= size())
    {
        throw Callable::invalid_argument();
    }
    return at(index);
}

template <typename Function>
auto Callable::Arguments::toTuple() const
{
    constexpr std::size_t N = function_traits<Function>::arity;
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        return std::tuple_cat(std::make_tuple(get<typename function_traits<Function>::object*>(0)),
                              ArgumentsToTuple<Function, 1>::template convert<N>(*this));
    }
    else
    {
        return ArgumentsToTuple<Function, 0>::template convert<N>(*this);
    }
}


template <typename Function>
Callable::Callable(Function fn)
    : m_ret(ArgumentDescriptor::get<typename function_traits<Function>::return_type>())
    , m_args(function_traits<Function>::argument_descriptors())
    , m_address(::address(fn))
    , m_type(static_cast<FunctionType>(function_traits<Function>::type))
    , m_isConst(function_traits<Function>::is_const)
{
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        m_classType = metaType<typename function_traits<Function>::object>();
    }

    m_invoker = [function = std::forward<Function>(fn)](const Arguments& args)
    {
        auto args_tuple = args.toTuple<Function>();

        if constexpr (std::is_void_v<typename function_traits<Function>::return_type>)
        {
            std::apply(function, args_tuple);
            return Argument();
        }
        else
        {
            auto ret = std::apply(function, args_tuple);
            return Argument(ret);
        }
    };
}


/******************************************************************************
 * invokes
 */

template <class Ret, typename... Arguments>
Ret invoke(const Callable& callable, Arguments... arguments)
{
    Callable::Arguments vargs(arguments...);
    Argument ret = callable.apply(vargs);
    if constexpr (!std::is_void_v<Ret>)
    {
        return ret;
    }
}

template <class Ret, class Class, typename... Arguments>
std::enable_if_t<std::is_class<Class>::value, Ret> invoke(const Callable& callable, Class& instance, Arguments... arguments)
{
    Callable::Arguments vargs(arguments...);
    Argument ret = callable.apply(instance, vargs);
    if constexpr (!std::is_void_v<Ret>)
    {
        return ret;
    }
}
} // namespace mox

#endif // CALLABLE_IMPL_HPP
