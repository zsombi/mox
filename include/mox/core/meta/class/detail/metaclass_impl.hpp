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

#ifndef METACLASS_IMPL_HPP
#define METACLASS_IMPL_HPP

#include <mox/core/meta/base/metabase.hpp>
namespace mox
{

namespace
{

template <class Class>
metainfo::MetaClass* ensureMetaClass()
{
    registerMetaClass<Class>();
    return Class::__getStaticMetaClass();
}

}

/******************************************************************************
 * register
 */
template <class ClassType>
std::pair<Metatype, Metatype> registerMetaClass(std::string_view name)
{
    auto typePair = registerClassMetaTypes<ClassType>(name);
    ClassType::StaticMetaClass::get();
    return typePair;
}

namespace metainfo
{

/******************************************************************************
 * MetaClass::MetaSignal
 */
template <class HostClass, typename... Arguments>
MetaClass::MetaSignal<HostClass, Arguments...>::MetaSignal(std::string_view name)
    : MetaSignalBase(*ensureMetaClass<HostClass>(), VariantDescriptorContainer::ensure<Arguments...>(), name)
{
}

template <class HostClass, typename... Arguments>
template <typename... ConvertibleArgs>
int MetaClass::MetaSignal<HostClass, Arguments...>::emit(MetaBase& sender, ConvertibleArgs... arguments)
{
    auto argPack = Callable::ArgumentPack(arguments...);
    return sender.activateSignal(*this, argPack);
}
/******************************************************************************
 * MetaClass::MetaProperty
 */
template <class HostClass, typename ValueType, PropertyAccess access>
MetaClass::MetaProperty<HostClass, ValueType, access>::MetaProperty(const MetaSignalBase& sigChanged, std::string_view name, const ValueType& defaultValue)
    : PropertyDefaultValue<ValueType>(defaultValue)
    , MetaPropertyBase(*ensureMetaClass<HostClass>(), VariantDescriptor::get<ValueType>(), access, sigChanged, *this, name)
{
}

/******************************************************************************
 * MetaClass::MetaMethod
 */
template <class HostClass>
template <typename MethodType>
MetaClass::MetaMethod<HostClass>::MetaMethod(MethodType method, std::string_view name)
    : MetaMethodBase(*ensureMetaClass<HostClass>(), method, name)
{
}

/******************************************************************************
 * StaticMetaClass
 */
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::StaticMetaClass()
    : mox::metainfo::MetaClass({mox::metaType<BaseClass>(), mox::metaType<BaseClass*>()})
{
    std::array<bool, 2 * sizeof...(SuperClasses)> casters =
    {{
        MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<BaseClass*, SuperClasses*>(), getMetaTypes().second, mox::registerClassMetaTypes<SuperClasses>().second)...,
        MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<SuperClasses*, BaseClass*>(), mox::metaType<SuperClasses*>(), getMetaTypes().second)...
    }};
    UNUSED(casters);
    MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<BaseClass*, MetaBase*>(), getMetaTypes().second, mox::registerClassMetaTypes<MetaBase>().second);
    MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<MetaBase*, BaseClass*>(), mox::metaType<MetaBase*>(), getMetaTypes().second);
}

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
MetaClass::VisitorResultType StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::visitSuperClasses(const MetaClassVisitor& visitor) const
{
    std::array<const mox::metainfo::MetaClass*, sizeof... (SuperClasses)> supers = {{SuperClasses::StaticMetaClass::get()...}};
    for (auto metaClass : supers)
    {
        VisitorResultType result = metaClass->visit(visitor);
        if (std::get<0>(result) == Abort)
        {
            return result;
        }
    }

    return std::make_tuple(Continue, MetaValue());
}

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
bool StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::isAbstract() const
{
    return std::is_abstract_v<BaseClass>;
}

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
bool StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::isClassOf(const MetaObject& metaObject) const
{
    const BaseClass* casted = dynamic_cast<const BaseClass*>(&metaObject);
    return casted != nullptr;
}

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
const typename StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::DeclaredMetaClass* StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::get()
{
    return BaseClass::__getStaticMetaClass();
}

/******************************************************************************
 * meta-invocators
 */

template <class Class, typename... Arguments>
int emit(Class& instance, std::string_view signalName, Arguments... arguments)
{
    const auto* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);
    // Metasignal lookup.
    auto signalVisitor = [signalName, &descriptors](const auto* signal, const auto& meta) -> bool
    {
        return (meta.name() == signalName) && signal->getArguments().isInvocableWith(descriptors);
    };
    auto signal = metaClass->visitSignals(signalVisitor);
    if (signal)
    {
        return instance.activateSignal(*signal, Callable::ArgumentPack(arguments...));
    }

    return -1;
}

template <class Class, typename... Arguments>
std::optional<Variant> invoke(Class& instance, std::string_view methodName, Arguments... arguments)
{
    const auto* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);

    // Metamethod lookup.
    auto methodVisitor = [methodName, &descriptors](const auto method, const auto& meta) -> bool
    {
        return (meta.name() == methodName) && method->isInvocableWith(descriptors);
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

template <typename ValueType, class Class>
std::optional<ValueType> getProperty(Class& instance, std::string_view property)
{
    const auto* metaClass = Class::StaticMetaClass::get();
    auto finder = [&property](const auto*, const auto& meta)
    {
        return meta.name() == property;
    };
    auto metaProperty = metaClass->visitProperties(finder);
    if (metaProperty)
    {
        ValueType value = instance.getProperty(*metaProperty);
        return std::make_optional(value);
    }
    return std::nullopt;
}

template <typename ValueType, class Class>
bool setProperty(Class& instance, std::string_view property, ValueType value)
{
    const auto* metaClass = Class::StaticMetaClass::get();
    auto finder = [&property](const auto*, const auto& meta)
    {
        return meta.name() == property;
    };
    auto metaProperty = metaClass->visitProperties(finder);
    if (metaProperty)
    {
        return instance.setProperty(*metaProperty, Variant(value)) != nullptr;
    }
    return false;
}

template <class Sender, class Receiver>
Signal::ConnectionSharedPtr connect(Sender& sender, std::string_view signal, Receiver& receiver, std::string_view slot)
{
    const auto* mcSender = Sender::StaticMetaClass::get();
    auto sigFinder = [signal](const auto, const auto& meta)
    {
        return meta.name() == signal;
    };
    auto metaSignal = mcSender->visitSignals(sigFinder);
    if (!metaSignal)
    {
        return nullptr;
    }
    auto sig = sender.findSignal(*metaSignal);
    if (!sig)
    {
        return nullptr;
    }

    const auto* mcReceiver = Receiver::StaticMetaClass::get();
    auto slotFinder = [slot, sig](const auto method, const auto& meta)
    {
        return (meta.name() == slot) && method->isInvocableWith(sig->getType()->getArguments());
    };
    auto metaSlot = mcReceiver->visitMethods(slotFinder);
    if (!metaSlot)
    {
        return nullptr;
    }

    return connect(*sig, receiver, *metaSlot);
}

} // metainfo

} // namespace mox

#endif // METACLASS_IMPL_HPP
