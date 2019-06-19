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

#ifndef FUNCTION_TRAITS_HPP
#define FUNCTION_TRAITS_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <vector>

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox
{

/// Defines the type of an argument. Callable holds the argument descriptors for the return type aswell
/// as for the callable arguments.
struct MOX_API ArgumentDescriptor
{
    /// Tye metatype of the argument.
    const Metatype type = Metatype::Invalid;
    /// \e true if the argument is a pointer, \e false if not.
    const bool isPointer = false;
    /// \e true if the argument is a reference, \e false if not.
    const bool isReference = false;
    /// \e true if the argument is a const, \e false if not.
    const bool isConst = false;

    /// Constructor.
    ArgumentDescriptor() = default;
    ArgumentDescriptor(Metatype type, bool ptr, bool ref, bool c)
        : type(type)
        , isPointer(ptr)
        , isReference(ref)
        , isConst(c)
    {
    }

    /// Returns the argument descriptor for the \e Type.
    /// \return The argument descriptor for the \e Type.
    template <typename Type>
    static ArgumentDescriptor&& get()
    {
        return std::move(ArgumentDescriptor{
                             metaType<Type>(),
                             std::is_pointer<Type>(),
                             std::is_reference<Type>(),
                             std::is_const<Type>()
                         });
    }
};

typedef std::vector<ArgumentDescriptor> ArgumentDescriptorContainer;

template <typename... Args>
ArgumentDescriptorContainer argument_descriptors()
{
    const std::array<ArgumentDescriptor, sizeof... (Args)> aa = {{ ArgumentDescriptor::get<Args>()... }};
    return ArgumentDescriptorContainer(aa.begin(), aa.end());
}

bool operator ==(const ArgumentDescriptor& arg1, const ArgumentDescriptor& arg2);
bool operator !=(const ArgumentDescriptor& arg1, const ArgumentDescriptor& arg2);


enum FunctionType
{
    /// Specifies an invalid function.
    Invalid = -1,
    /// The callable is a function.
    Function,
    /// The callable is a functor.
    Functor,
    /// The callable is a method.
    Method
};

/// Traits for functors and function objects.
template <typename Function>
struct function_traits : public function_traits<decltype(&Function::operator())>
{
    enum { type = FunctionType::Functor };
};

/// Method traits.
template <class TObject, typename TRet, typename... Args>
struct function_traits<TRet(TObject::*)(Args...)>
{
    using object = TObject;
    using return_type = TRet;
    typedef TRet(TObject::*function_type)(Args...);

    static constexpr std::size_t arity = sizeof... (Args);
    enum { is_const = false };
    enum { type = FunctionType::Method };

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    static std::vector<ArgumentDescriptor> argument_descriptors()
    {
        return mox::argument_descriptors<Args...>();
    }
};

/// Const method traits.
template <class TObject, typename TRet, typename... Args>
struct function_traits<TRet(TObject::*)(Args...) const>
{
    using object = TObject;
    using return_type = TRet;
    typedef TRet(TObject::*function_type)(Args...) const;

    static constexpr std::size_t arity = sizeof... (Args);
    enum { is_const = true };
    enum { type = FunctionType::Method };

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    static std::vector<ArgumentDescriptor> argument_descriptors()
    {
        return mox::argument_descriptors<Args...>();
    }
};

/// Function and static member function traits.
template <typename TRet, typename... Args>
struct function_traits<TRet(*)(Args...)>
{
    using return_type = TRet;
    typedef TRet(*function_type)(Args...);

    static constexpr std::size_t arity = sizeof... (Args);
    enum { is_const = false };
    enum { type = FunctionType::Function };

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    static std::vector<ArgumentDescriptor> argument_descriptors()
    {
        return mox::argument_descriptors<Args...>();
    }
};

} // namespace mox

#endif // FUNCTION_TRAITS_HPP
