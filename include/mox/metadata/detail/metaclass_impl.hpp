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

namespace mox
{

template <typename Function>
MetaClass::Method::Method(MetaClass& metaClass, Function fn, std::string_view name)
    : Callable(fn)
    , m_ownerClass(metaClass)
    , m_name(name)
{
    m_ownerClass.addMethod(*this);
}

template <class SenderObject, typename... Arguments>
int MetaClass::Signal::emit(SenderObject& sender, Arguments... args) const
{
    if (!isInvocableWith(VariantDescriptorContainer::get<Arguments...>()))
    {
        return -1;
    }
    return activate(reinterpret_cast<intptr_t>(&sender), Callable::ArgumentPack(args...));
}


template <class Class, typename... Arguments>
std::optional<Variant> MetaClass::invoke(Class& instance, const Method& method, Arguments... arguments) const
{
    if (!method.isInvocableWith(VariantDescriptorContainer(arguments...)))
    {
        return std::nullopt;
    }

    try
    {
        auto argPack = (method.type() == FunctionType::Method)
                ? Callable::ArgumentPack(&instance, arguments...)
                : Callable::ArgumentPack(arguments...);

        auto result = method.apply(argPack);
        return std::make_optional(result);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

/******************************************************************************
 * StaticMetaClass
 */
namespace
{

template <typename From, typename To>
struct Caster : MetatypeConverter
{
    static MetaValue converter(const MetatypeConverter&, const void* value)
    {
        auto src = const_cast<From*>(reinterpret_cast<const From*>(value));
        auto dst = dynamic_cast<To*>(src);
        return dst;
    }

    explicit Caster()
        : MetatypeConverter(converter)
    {
    }
};

} // noname

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::StaticMetaClass()
    : mox::MetaClass(metatypeDescriptor<BaseClass>())
{
    struct Element
    {
        MetatypeConverter* converter;
        Metatype from;
        Metatype to;
    };

    std::array<Element, 2 * sizeof...(SuperClasses)> casters =
    {{
        Element{new Caster<SuperClasses, BaseClass>(), mox::metaType<SuperClasses*>(), mox::metaType<BaseClass*>()}...,
        Element{new Caster<BaseClass, SuperClasses>(), mox::metaType<BaseClass*>(), mox::metaType<SuperClasses*>()}...
    }};
    for (auto& caster : casters)
    {
        metadata::registerConverter(std::unique_ptr<MetatypeConverter>(caster.converter), caster.from, caster.to);
    }
}

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
MetaClass::VisitorResultType StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::visitSuperClasses(const MetaClassVisitor& visitor) const
{
    std::array<const mox::MetaClass*, sizeof... (SuperClasses)> supers = {{SuperClasses::StaticMetaClass::get()...}};
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

/******************************************************************************
 * helpers
 */
template <class ClassType>
Metatype registerMetaClass(std::string_view name)
{
    Metatype type = metadata::findMetatype(metadata::remove_cv<ClassType>());
    if (type != Metatype::Invalid)
    {
        ClassType::StaticMetaClass::get();
        return type;
    }

    type = registerMetaType<ClassType>(name);
    typedef std::add_pointer_t<ClassType> PtrClassType;
    std::string pname(name);
    if (!pname.empty())
    {
        pname += "*";
    }
    registerMetaType<PtrClassType>(pname);
    ClassType::StaticMetaClass::get();
    return type;
}

template <class Class, typename... Arguments>
std::optional<Variant> invoke(Class& instance, std::string_view methodName, Arguments... arguments)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);

    // Metamethod lookup.
    auto methodVisitor = [methodName, &descriptors](const MetaClass::Method* method) -> bool
    {
        return (method->name() == methodName) && method->isInvocableWith(descriptors);
    };
    const MetaClass::Method* metaMethod = metaClass->visitMethods(methodVisitor);
    if (metaMethod)
    {
        return metaClass->invoke(instance, *metaMethod, arguments...);
    }

    return std::nullopt;
}

template <class Class, typename... Arguments>
int emit(Class& instance, std::string_view signalName, Arguments... arguments)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);
    // Metasignal lookup.
    auto signalVisitor = [signalName, &descriptors](const MetaClass::Signal* signal) -> bool
    {
        return (signal->name() == signalName) && signal->isInvocableWith(descriptors);
    };
    const MetaClass::Signal* signal = metaClass->visitSignals(signalVisitor);
    if (signal)
    {
        return signal->activate(reinterpret_cast<intptr_t>(&instance), Callable::ArgumentPack(arguments...));
    }

    return -1;
}

} // namespace mox

#endif // METACLASS_IMPL_HPP
