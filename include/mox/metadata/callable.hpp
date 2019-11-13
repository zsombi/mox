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

#ifndef CALLABLE_HPP
#define CALLABLE_HPP

#include <functional>
#include <tuple>
#include <type_traits>

#include <mox/metadata/variant.hpp>
#include <mox/utils/function_traits.hpp>
#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox
{

/// Exception thrown on invalid argument count, or invalid argument type.
class MOX_API invalid_argument : public std::exception
{
public:
    /// Constructor.
    explicit invalid_argument() = default;

    /// String representation of the exception.
    const char* what() const EXCEPTION_NOEXCEPT override;
};

/******************************************************************************
 * Callables
 */

/// The class represents a callable in Mox.
class MOX_API Callable
{
    using ArgumentsBase = std::vector<Variant>;

public:
    /// ArgumentPack is a container that holds the argument value and its descriptor packed.
    /// The packed arguments are trasported across threads keeping the lifetime of the values.
    struct MOX_API ArgumentPack : protected ArgumentsBase
    {
        ArgumentPack() = default;

        /// Constructs an argument pack by catenating an instance and an other argument
        /// pack.
        template <class Instance>
        ArgumentPack(Instance instance, const ArgumentPack& other);

        /// Packs the \a arguments.
        template <typename... Args>
        ArgumentPack(Args... arguments);

        /// Returns the argument from a given \a index that is less than the count().
        /// \param index The index of the argument requested.
        /// \return The argument value.
        /// \throws std::bad_any_cast if the argument at \a index is of a different type.
        /// \throws invalid_argument if the argument \a index is out of bounds.
        template <typename Type>
        Type get(size_t index) const;

        /// Repacks the argument pack into a tuple.
        template <typename Function>
        auto toTuple() const;
    };

    /// Invoker function type.
    typedef std::function<Variant(ArgumentPack const&)> InvokerFunction;

    /// Creates a callable for a function, method or functor.
    template <typename Function>
    Callable(Function fn);

    /// Move constructor.
    Callable(Callable&& other);

    /// Move operator.
    Callable& operator=(Callable&&);

    /// Comparison operator.
    bool operator==(const Callable& other) const;

    /// Returns the type of the callable.
    /// \return The type of the callable.
    /// \see Type.
    FunctionType type() const;

    /// Checks if the callable is const.
    /// \return \e true if the callable is const, \e false if not.
    /// \note Lambdas are const!
    bool isConst() const;

    /// Returns the argument descriptor for the return type of the callable.
    /// \return The argument descriptor for the return type.
    const VariantDescriptor& returnType() const;

    /// Returns the metatype of the class that owns the callable method.
    /// \return The metatype of the class, Invalid if the callable is not holding a method.
    Metatype classType() const;

    /// Returns the number of arguments of the callable.
    /// \return The number of arguments of the callable.
    size_t argumentCount() const;

    /// Returns the argument descriptor for the argument at \a index.
    /// \return the argument descriptor for the argument at \a index.
    /// \throws Callable::invalid_argument if the \a index is out of arguments bounds.
    const VariantDescriptor& argumentType(size_t index) const;

    /// Returns the container with the argument descriptors.
    /// \return The argument descriptor container of the callable.
    const VariantDescriptorContainer& descriptors() const;

    /// Tests whether the callable is invocable with the passed \a arguments.
    /// \return If the callable is invocable with the arguments, returns \e true,
    /// otherwise \e false
    bool isInvocableWith(const VariantDescriptorContainer& arguments) const;

    /// Applies the arguments on a callable.
    /// \param args The arguments to apply. The collection must have at least as many arguments as many
    /// formal parameters are defined for the callable. When the callable is a method, the first argument
    /// must be the instance of the class the method belongs to.
    /// \return The return value of the callable, or an invalid \l Variant if the callable is a void type.
    /// \throws Callable::invalid_argument if the argument count is less than the defined argument count
    /// for the callable.
    /// \throws bad_conversion if the arguments mismatch.
    Variant apply(const ArgumentPack& args) const;

    /// Resets the callable.
    void reset();

    /// Swaps two callables.
    void swap(Callable& other);

private:
    InvokerFunction m_invoker;
    VariantDescriptor m_ret;
    VariantDescriptorContainer m_args;
    Metatype m_classType = Metatype::Invalid;
    FunctionType m_type = FunctionType::Invalid;
    bool m_isConst = false;

    DISABLE_COPY(Callable)
};

} // namespace mox

#include <mox/metadata/detail/callable_impl.hpp>

#endif // CALLABLE_HPP
