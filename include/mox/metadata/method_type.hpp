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

#ifndef METHOD_TYPE_HPP
#define METHOD_TYPE_HPP

#include <optional>
#include <mox/config/deftypes.hpp>
#include <mox/metadata/variant.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <mox/metadata/metaclass.hpp>

namespace mox
{

/// The MethodType declares a callable on a function of a class. You can set methods, static methods,
/// functions or lambdas as MethodTypes to your class. The MethodType gets registered to the MetaClass
/// of your class, and can only be declared inside the class, or the metaclass of that class. Unlike
/// the SignalType and PropertyType, MethodType declaration without MetaClass does not make sense, and
/// will cause build failure.
class MOX_API MethodType : public Callable, public AbstractMetaInfo
{
public:
    /// Builds the signature of the method.
    std::string signature() const override;

protected:
    /// Constructor.
    template <typename Function>
    MethodType(Function method, std::string_view name)
        : Callable(method)
        , AbstractMetaInfo(name)
    {
    }
};

/// MethodType declarator template class. Use this to declare method types for your metaclass.
/// \tparam HostClass The class owning the method and hosting a MetaClass.
template <class HostClass>
class MethodTypeDecl : public MethodType
{
public:
    /// Constructor.
    /// \tparam Function The function type of the method
    /// \param method The method function that is registered to the MetaClass.
    /// \param name The name of the method.
    template <typename Function>
    explicit MethodTypeDecl(Function method, std::string name)
        : MethodType(method, name)
    {
        static_assert(has_static_metaclass_v<HostClass>, "MethodType without a class with metadata.");
        HostClass::__getStaticMetaClass()->addMetaMethod(*this);
    }
};

/// Invokes a method on an \a instance, passing the given \a arguments. The instance must
/// have a metaclass defined.
/// \param instance The instance of the class.
/// \param methodName The name of the metamethod to invoke.
/// \param arguments The arguments to pass. If the invokable has no arguments, pass nothing.
/// \returns If the metamethod is found on the instance, returns \e true, otherwise \e false.
template <class Class, typename... Arguments>
std::optional<Variant> invoke(Class& instance, std::string_view methodName, Arguments... arguments)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);

    // Metamethod lookup.
    auto methodVisitor = [methodName, &descriptors](const auto method) -> bool
    {
        return (method->name() == methodName) && method->isInvocableWith(descriptors);
    };
    const auto metaMethod = metaClass->visitMethods(methodVisitor);
    if (metaMethod)
    {
        try
        {
            auto argPack = (metaMethod->type() == FunctionType::Method)
                    ? Callable::ArgumentPack(&instance, arguments...)
                    : Callable::ArgumentPack(arguments...);

            auto result = metaMethod->apply(argPack);
            return std::make_optional(result);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

} // namespace mox

#endif // METHOD_TYPE_HPP
