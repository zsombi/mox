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

#include <mox/metadata/argument.hpp>
#include <mox/utils/function_traits.hpp>
#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox
{

/******************************************************************************
 * Callables
 */

/// The class represents a callable in Mox.
class MOX_API Callable
{
public:
    /// Exception thrown on invalid argument count, or invalid argument type.
    class MOX_API invalid_argument : public std::exception
    {
    public:
        /// Constructor.
        explicit invalid_argument();

        /// String representation of the exception.
        const char* what() const EXCEPTION_NOEXCEPT override;
    };

    /// Argument value container.
    struct MOX_API Arguments : protected std::vector<Argument>
    {
        /// Argument base type.
        typedef std::vector<Argument> ArgumentsBase;

        Arguments() = default;
        Arguments(const Arguments&) = default;
        Arguments(Arguments&&) = default;

        /// Packs the \a arguments passed.
        template <typename... Args>
        Arguments(Args... arguments);

        /// Adds an argument \a value of a \e Type.
        /// \param value The argument value to pass.
        /// \return This argument object.
        template <typename Type>
        Arguments& add(const Type& value);

        /// Sets the instance\a value to the argument pack.
        /// \param value The instance value to set to the argument pack.
        /// \return This argument object.
        template <typename Type>
        Arguments& setInstance(Type value);

        /// Returns the argument from a given \a index that is less than the count().
        /// \param index The index of the argument requested.
        /// \return The argument value.
        /// \throws std::bad_any_cast if the argument at \a index is of a different type.
        /// \throws Callable::invalid_argument if the argument \a index is out of bounds.
        template <typename Type>
        Type get(size_t index) const;

        /// Returns the number of argument values from the container.
        /// \return The number of argument values.
        size_t count() const;

        /// Repacks the argument package into a tuple.
        template <typename Function>
        auto toTuple() const;

        /// Returns the argument descriptors defining the arguments.
        const ArgumentDescriptorContainer descriptors() const;

        Arguments& operator +=(const Arguments& other);
    };

    /// Invoker function type.
    typedef std::function<Argument(Arguments const&)> InvokerFunction;

    /// Creates a callable for a function, method or functor.
    template <typename Function>
    Callable(Function fn);

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
    const ArgumentDescriptor& returnType() const;

    /// Returns the metatype of the class that owns the callable method.
    /// \return The metatype of the class, Invalid if the callable is not holding a method.
    Metatype classType() const;

    /// Returns the number of arguments of the callable.
    /// \return The number of arguments of the callable.
    size_t argumentCount() const;

    /// Returns the argument descriptor for the argument at \a index.
    /// \return the argument descriptor for the argument at \a index.
    /// \throws Callable::invalid_argument if the \a index is out of arguments bounds.
    const ArgumentDescriptor& argumentType(size_t index) const;

    /// Returns the container with the argument descriptors.
    /// \return The argument descriptor container of the callable.
    const ArgumentDescriptorContainer& descriptors() const;

    /// Applies the arguments on a callable.
    /// \param args The arguments to apply. The collection must have at least as many arguments as many
    /// formal parameters are defined for the callable. When the callable is a method, the first argument
    /// must be the instance of the class the method belongs to.
    /// \return The return value of the callable, or an invalid \l Argument if the callable is a void type.
    /// \throws Callable::invalid_argument if the argument count is less than the defined argument count
    /// for the callable.
    /// \throws bad_conversion if the arguments mismatch.
    Argument apply(const Arguments& args) const;

    template <class Class>
    Argument apply(Class& instance, const Arguments& args) const
    {
        Arguments thisArgs(args);
        thisArgs.setInstance(&instance);
        return apply(thisArgs);
    }

    /// Returns the address of the callable.
    /// \return The arrdess of the callable. nullptr is returned when the callable is a lambda.
    const void* address() const;

    /// Resets the callable.
    void reset();

private:
    InvokerFunction m_invoker;
    ArgumentDescriptor m_ret;
    ArgumentDescriptorContainer m_args;
    void* m_address = nullptr;
    Metatype m_classType = Metatype::Invalid;
    FunctionType m_type = FunctionType::Invalid;
    bool m_isConst = false;
};

/// Invokes a \a callable with \a arguments. If the callable has a return value, returns that.
/// Packs the \a arguments and applies those on the \a callable.
/// If the callable is a method of a class, pass the pointer to the instance of the class as
/// first argument. When you do that, make sure the instance is casted to the class that hosts
/// the method managed by the callable.
/// \param callable The callable to invoke.
/// \param arguments The arguments to pass to the callable.
/// \return The return value of the callable, undefined if the callable has no return value.
template <class Ret, typename... Arguments>
Ret invoke(const Callable& callable, Arguments... arguments);

/// Template function specialized on callables when the first argument is a class. Pass the
/// instance as reference to use the template.
/// \param callable The callable to invoke.
/// \param instance The instance of the class invoked.
/// \param arguments The arguments to pass to the callable.
/// \return The return value of the callable, undefined if the callable has no return value.
template <class Ret, class Class, typename... Arguments>
std::enable_if_t<std::is_class<Class>::value, Ret> invoke(const Callable& callable, Class& instance, Arguments... arguments);

/// Returns \e true if the \a arguments are compatible with the \a parameters
/// \return If the \a parameters size is at least the size of the \a arguments, and the descriptors
/// at the positions are compatible, returns \e true. Otherwise returns \e false.
MOX_API bool isCallableWith(const Callable& callable, const ArgumentDescriptorContainer& parameters);

} // namespace mox

#include <mox/metadata/detail/callable_impl.hpp>

#endif // CALLABLE_HPP
