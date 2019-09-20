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

template <class Class, typename... Arguments>
bool MetaClass::invoke(Class& instance, const Method& method, Arguments... arguments) const
{
    if (!method.isInvocableWith(VariantDescriptorContainer(arguments...)))
    {
        return false;
    }

    Callable::ArgumentPack argPack(arguments...);
    if (method.type() == FunctionType::Method)
    {
        argPack.setInstance(&instance);
    }

    method.apply(argPack);
    return true;
}

template <typename Ret, class Class, typename... Arguments>
bool MetaClass::invoke(Class& instance, Ret& retVal, const Method& method, Arguments... arguments) const
{
    // Remove reference from retVal type, as retVal is made a ref type to extract the return value.
    VariantDescriptor retDes(retVal);// = VariantDescriptor::get<Ret>();
    if (!method.isInvocableWith(VariantDescriptorContainer(arguments...)) || !retDes.invocableWith(method.returnType()))
    {
        return Ret();
    }

    Callable::ArgumentPack argPack(arguments...);
    if (method.type() == FunctionType::Method)
    {
        argPack.setInstance(&instance);
    }

    Variant result = method.apply(argPack);
    retVal = result;
    return true;
}

template <typename Class, typename... Arguments>
int MetaClass::emit(Class& instance, const Signal& signal, Arguments... arguments) const
{
    if (!signal.isInvocableWith(VariantDescriptorContainer(arguments...)))
    {
        return -1;
    }
    Callable::ArgumentPack argPack(arguments...);
    return instance.activate(signal.descriptor(), argPack);
}

template <class ClassType>
Metatype registerMetaClass(std::string_view name)
{
    Metatype type = registrar::findMetatype(registrar::remove_cv<ClassType>());
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
bool metaInvoke(Class& instance, std::string_view methodName, Arguments... arguments)
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
        metaClass->invoke(instance, *metaMethod, arguments...);
    }

    return (metaMethod != nullptr);
}

template <typename Ret, class Class, typename... Arguments>
bool metaInvoke(Class& instance, Ret& retVal, std::string_view methodName, Arguments... arguments)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    VariantDescriptorContainer descriptors(arguments...);

    // Metamethod lookup.
    auto methodVisitor = [methodName, retType = metaType<Ret>(), &descriptors](const MetaClass::Method* method) -> bool
    {
        return (method->name() == methodName) && (method->returnType().type == retType) && method->isInvocableWith(descriptors);
    };
    const MetaClass::Method* metaMethod = metaClass->visitMethods(methodVisitor);
    if (metaMethod)
    {
        return metaClass->invoke(instance, retVal, *metaMethod, arguments...);
    }
    return false;
}

template <class Class, typename... Arguments>
int metaEmit(Class& instance, std::string_view signalName, Arguments... arguments)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    if constexpr (std::is_base_of<MetaObject, Class>::value)
    {
        metaClass = instance.getMetaClass();
    }

    VariantDescriptorContainer descriptors(arguments...);
    // Metasignal lookup.
    auto signalVisitor = [signalName, &descriptors](const MetaClass::Signal* signal) -> bool
    {
        return (signal->name() == signalName) && signal->isInvocableWith(descriptors);
    };
    const MetaClass::Signal* signal = metaClass->visitSignals(signalVisitor);
    if (signal)
    {
        return const_cast<MetaClass*>(metaClass)->emit(instance, *signal, arguments...);
    }

    return -1;
}

} // namespace mox

#endif // METACLASS_IMPL_HPP
