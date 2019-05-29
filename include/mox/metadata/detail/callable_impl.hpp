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
    static auto convert(const Callable::Arguments& arguments)
    {
        typedef typename function_traits<Function>::template argument<Index - 1>::type ArgType;
        return std::tuple_cat(convert<Index - 1>(arguments), std::make_tuple(arguments.get<ArgType>(Index - 1 + Offset)));
    }

    template <>
    static auto convert<0>(const Callable::Arguments& /*arguments*/)
    {
        return std::tuple<>();
    }
};

} // noname

template <typename... Args>
Callable::Arguments::Arguments(Args... arguments)
{
    std::array<std::any, sizeof... (Args)> aa = {{std::any(arguments)...}};
    insert(begin(), aa.begin(), aa.end());
}

template <typename Type>
Callable::Arguments& Callable::Arguments::add(const Type& value)
{
    push_back(std::any(value));
    return *this;
}

template <typename Type>
Callable::Arguments& Callable::Arguments::prepend(Type value)
{
    insert(begin(), std::any(value));
    return *this;
}

template <typename Type>
Type Callable::Arguments::get(size_t index) const
{
    if (index >= size())
    {
        throw Callable::invalid_argument();
    }
    return std::any_cast<Type>(operator[](index));
}

template <typename Function>
auto Callable::Arguments::toTuple() const
{
    constexpr std::size_t N = function_traits<Function>::arity;
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        return std::tuple_cat(std::make_tuple(get<typename function_traits<Function>::object*>(0)),
                              AnyToTuple<Function, 1>::template convert<N>(*this));
    }
    else
    {
        return AnyToTuple<Function, 0>::template convert<N>(*this);
    }
}


template <typename Function>
Callable::Callable(Function fn)
    : m_ret(ArgumentDescriptor::get<typename function_traits<Function>::return_type>())
    , m_args(function_traits<Function>::argument_descriptors())
    , m_type(static_cast<FunctionType>(function_traits<Function>::type))
    , m_isConst(function_traits<Function>::is_const)
{
    if constexpr (function_traits<Function>::type == FunctionType::Method)
    {
        m_classType = MetaTypeDescriptor::typeId<typename function_traits<Function>::object>();
    }

    m_invoker = [function = std::forward<Function>(fn)](const Arguments& args) -> std::any
    {
        auto args_tuple = args.toTuple<Function>();

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

template <class Ret, typename... Arguments>
Ret invoke(const Callable& callable, Arguments... arguments)
{
    Callable::Arguments vargs(arguments...);
    std::any ret = callable.apply(vargs);
    if constexpr (!std::is_void<Ret>::value)
    {
        return std::any_cast<Ret>(ret);
    }
}

template <class Ret, class Class, typename... Arguments>
typename std::enable_if<std::is_class<Class>::value, Ret>::type invoke(const Callable& callable, Class& instance, Arguments... arguments)
{
    Callable::Arguments vargs(arguments...);
    std::any ret = callable.apply(instance, vargs);
    if constexpr (!std::is_void<Ret>::value)
    {
        return std::any_cast<Ret>(ret);
    }
}
} // namespace mox

#endif // CALLABLE_IMPL_HPP
