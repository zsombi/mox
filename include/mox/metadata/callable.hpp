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

#include <any>
#include <functional>
#include <tuple>
#include <type_traits>

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
        const char* what() const _NOEXCEPT override;
    };

    /// Argument value container.
    struct MOX_API Arguments : protected std::vector<std::any>
    {
        /// Argument base type.
        typedef std::vector<std::any> ArgumentsBase;

        Arguments() = default;
        Arguments(const Arguments&) = default;
        Arguments(Arguments&&) = default;

        template <typename _Iterator>
        Arguments(_Iterator ibegin, _Iterator iend)
            : ArgumentsBase(ibegin, iend)
        {}

        /// Adds an argument \a value of a \e Type.
        /// \param value The argument value to pass.
        /// \return This argument object.
        template <typename Type>
        Arguments& add(const Type& value)
        {
            push_back(std::any(value));
            return *this;
        }

        /// Returns the argument from a given \a index that is less than the count().
        /// \param index The index of the argument requested.
        /// \return The argument value.
        /// \throws std::bad_any_cast if the argument at \a index is of a different type.
        /// \throws Callable::invalid_argument if the argument \a index is out of bounds.
        template <typename Type>
        Type get(size_t index)
        {
            if (index >= size())
            {
                throw Callable::invalid_argument();
            }
            return std::any_cast<Type>(operator[](index));
        }

        /// Returns the number of argument values from the container.
        /// \return The number of argument values.
        size_t count() const
        {
            return size();
        }
    };

    /// Container for the argument types.
    typedef std::vector<ArgumentDescriptor> ArgumentDescriptorContainer;
    /// Invoker function type.
    typedef std::function<std::any(Arguments)> InvokerFunction;

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
    MetaType::TypeId classType() const;

    /// Returns the number of arguments of the callable.
    /// \return The number of arguments of the callable.
    size_t argumentCount() const;

    /// Returns the argument descriptor for the argument at \a index.
    /// \return the argument descriptor for the argument at \a index.
    /// \throws Callable::invalid_argument if the \a index is out of arguments bounds.
    const ArgumentDescriptor& argumentType(size_t index) const;

    /// Applies the arguments on a callable.
    /// \param args The arguments to apply. The collection must have at least as many arguments as many
    /// formal parameters are defined for the callable. When the callable is a method, the first argument
    /// must be the instance of the class the method belongs to.
    /// \return The return value of the callable, or an invalid \c std::any if the callable is a void type.
    /// \throws Callable::invalid_argument if the argument count is less than the defined argument count
    /// for the callable.
    /// \throws std::bad_any_cast if the arguments mismatch.
    std::any apply(const Arguments& args) const;

private:
    InvokerFunction m_invoker;
    ArgumentDescriptor m_ret;
    ArgumentDescriptorContainer m_args;
    MetaType::TypeId m_classType = MetaType::TypeId::Invalid;
    FunctionType m_type = FunctionType::Invalid;
    bool m_isConst = false;
};

/// Invokes a callable with arguments. If the callable has a return value, returns that.
/// Packs the \a arguments and applies those on the \a callable.
/// \param callable The callable to invoke.
/// \param arguments The arguments to pass to the callable.
/// \return The return value of the callable, undefined if the callable has no return value.
template <typename Ret, typename... Arguments>
Ret invoke(const Callable& callable, Arguments... arguments);

/// Invokes a \a callable on an \a instance with \a arguments. If the callable has a return
/// value, returns that. Packs the \a instance with the \a arguments and applies those on the
/// \a callable.
/// \param instance The instance the callable belongs to.
/// \param callable The callable to invoke.
/// \param arguments The arguments to pass to the callable.
/// \return The return value of the callable, undefined if the callable has no return value.
template <typename Ret, class Class, typename... Arguments>
Ret invoke(Class* instance, const Callable& callable, Arguments... arguments);

} // namespace mox

#include <mox/metadata/detail/callable_impl.hpp>

#endif // CALLABLE_HPP
