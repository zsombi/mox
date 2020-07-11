// Copyright (C) 2020 bitWelder

#ifndef ARGUMENT_DATA_HPP
#define ARGUMENT_DATA_HPP

#include <any>
#include <mox/config/deftypes.hpp>
#include <mox/config/error.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/utils/function_traits.hpp>

namespace mox
{

/// ArgumentData stores an argument passed on slot invocation.
class MOX_API ArgumentData : public std::any
{
public:
    /// Creates a default argument data, with no data stored.
    ArgumentData() = default;

    /// Creates an argument data with a value.
    /// \tparam T The type of the value passed as argument.
    /// \param value The value to store.
    template <class T>
    ArgumentData(T value)
        : std::any(value)
    {
    }

    /// Cast operator, returns the data stored by an ArgumentData instance.
    /// \tparam T The type of the casted value.
    /// \return The value stored.
    /// \throws Throws std::bad_any_cast if the type to cast to is not the type the data is stored.
    template <class T>
    operator T() const
    {
        return std::any_cast<T>(*this);
    }
};

/// The PackedArguments class packs values passed as arguments to a signal or a slot invocation.
struct MOX_API PackedArguments : protected std::vector<ArgumentData>
{
    /// Creates an argument pack with the \a arguments.
    /// \tparam Arguments Variadic number of arguments to pack.
    /// \param arguments The variadic argument values to pack.
    template <typename... Arguments>
    PackedArguments(Arguments&&... arguments);

    PackedArguments& operator+=(const ArgumentData& data)
    {
        push_back(data);
        return *this;
    }

    /// Gets the value of an argument at a given \a index.
    /// \tparam T The argument type to get. The argument type must be identical to the type the
    /// type of the stored value.
    /// \param index The index of the argument.
    /// \return The value of the argument at index.
    /// \throws Exception with ExceptionType::InvalidArgument if the \a index is greater than the
    /// packed argument value count.
    template <typename T>
    T get(size_t index) const;

    /// Repacks the argument pack into a tuple, using the signature of a function
    /// \tparam FunctionSignature The function signature to use in repacking
    /// \param instance The instance of the method to call with.
    /// \return The tuple prepared with the arguments ready to invoke a callable.
    /// \throws ExceptionType::InvalidArgument when the function signature requires more arguments
    /// than it is available in the package.
    /// \throws std::bad_any_cast when the argument types of the signature do not match the type
    /// of the argument value stored in the package.
    template <typename FunctionSignature>
    auto repack(typename function_traits<FunctionSignature>::object* instance) const;

    /// Repacks the argument pack into a tuple, using the signature of a function
    /// \tparam FunctionSignature The function signature to use in repacking
    /// \return The tuple prepared with the arguments ready to invoke a callable.
    /// \throws ExceptionType::InvalidArgument when the function signature requires more arguments
    /// than it is available in the package.
    /// \throws std::bad_any_cast when the argument types of the signature do not match the type
    /// of the argument value stored in the package.
    template <class FunctionSignature>
    auto repack() const;

private:
    template <typename Function>
    struct PackToTuple
    {
        template <int Index>
        static auto convert(const PackedArguments& arguments)
        {
            if constexpr (Index == 0)
            {
                return std::tuple<>();
            }
            else
            {
                using ArgType = typename function_traits<Function>::template argument<Index - 1>::type;
                return std::tuple_cat(convert<Index - 1>(arguments), std::make_tuple(arguments.get<ArgType>(Index - 1)));
            }
        }
    };
};

/******************************************************************************
 * Implementation
 */
template <typename... Arguments>
PackedArguments::PackedArguments(Arguments&&... arguments)
{
    std::array<ArgumentData, sizeof... (Arguments)> aa = {{ArgumentData(arguments)...}};
    reserve(aa.size());
    insert(end(), aa.begin(), aa.end());
}

template <typename T>
T PackedArguments::get(size_t index) const
{
    if (index >= size())
    {
        throw Exception(ExceptionType::InvalidArgument);
    }
    return at(index);
}

template <typename FunctionSignature>
auto PackedArguments::repack(typename function_traits<FunctionSignature>::object* instance) const
{
    constexpr std::size_t N = function_traits<FunctionSignature>::arity;
    return std::tuple_cat(std::make_tuple(static_cast<typename function_traits<FunctionSignature>::object*>(instance)),
                          PackToTuple<FunctionSignature>::template convert<N>(*this));
}

template <class FunctionSignature>
auto PackedArguments::repack() const
{
    constexpr std::size_t N = function_traits<FunctionSignature>::arity;
    return PackToTuple<FunctionSignature>::template convert<N>(*this);
}


} // mox

#endif // ARGUMENT_DATA_HPP
