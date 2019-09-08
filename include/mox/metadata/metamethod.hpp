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

#ifndef METAMETHOD_HPP
#define METAMETHOD_HPP

#include <string>
#include <string_view>
#include <type_traits>
#include <optional>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metaclass.hpp>

namespace mox
{

class MOX_API metamethod_not_found : public std::exception
{
public:
    explicit metamethod_not_found(std::string_view method)
        : m_method(method)
    {
    }

    const char* what() const EXCEPTION_NOEXCEPT override
    {
        std::string msg("metamethod not declared on class: ");
        msg += m_method;
        return msg.c_str();
    }
private:
    std::string_view m_method;
};

/// MetaMethod is a callable that is attached to a MetaClass, and typically holds a method of a class.
/// However, you can add static methods, functions or lambdas to the metaclass of your class, declared
/// outside of the class' scope. These functions do not get passed the class instance they are invoked
/// on, however invoking these types of metamethods require the class instance.
///
/// MetaMethods are invoked using invokeMethod() template function, passing the instance, the name of
/// the method and the eventual arguments that are forwarded to the method.
class MOX_API MetaMethod : public Callable
{
public:
    /// Constructs a metamethod with the \a function identified with \a name, and attaches to the \a metaClass.
    template <typename Function>
    explicit MetaMethod(MetaClass& metaClass, Function fn, std::string_view name);

    /// Returns the name of the metamethod.
    /// \return The name of the metamethod.
    std::string name() const
    {
        return m_name;
    }

    /// Returns the metaclass that owns the metamethod.
    /// \return The metaclass that owns the metamethod.
    const MetaClass& ownerClass() const
    {
        return m_ownerClass;
    }

private:
    MetaClass& m_ownerClass;
    std::string m_name;

    MetaMethod(MetaMethod const&) = delete;
    MetaMethod(MetaMethod&&) = delete;
};

/// Invokes a \a method on an \a instance, passing the given \a arguments. The instance must have
/// a metaclass defined. If the method has a return value, returns that. If the requested return
/// type is void, the return value is ignored.
/// \param instance The instance of the class.
/// \param method The name of the method to invoke.
/// \param args The arguments to pass. If the method has no arguments, pass nothing.
/// \returns Returns the
/// \throws mox::metamethod_not_found if the metamethod invoked is not declared for the metaclass of the instance.
/// \throws std::bad_any_cast if the metamethod returns void, but a different return value is requested,
/// or one of the passed argument type differs from the declared one.
template <typename Ret, class Class, typename... Arguments>
Ret invokeMethod(Class& instance, std::string_view method, Arguments... args);

} // namespace mox

#include <mox/metadata/detail/metamethod_impl.hpp>

#define META_METHOD(Class, name)  mox::MetaMethod name{*this, &Class::name, #name}

#endif // METAMETHOD_HPP
