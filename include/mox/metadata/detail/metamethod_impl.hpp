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

#ifndef METAMETHOD_IMPL_HPP
#define METAMETHOD_IMPL_HPP

namespace mox
{

template <typename Function>
MetaMethod::MetaMethod(MetaClass& metaClass, Function fn, std::string_view name)
    : Callable(fn)
    , m_ownerClass(metaClass)
    , m_name(name)
{
    metaClass.addMethod(this);
}


template <typename Ret, class Class, typename... Arguments>
Ret invokeMethod(Class& instance, std::string_view method, Arguments... args)
{
    const MetaClass* metaClass = instance.getStaticMetaClass();
    if constexpr (std::is_base_of<MetaObject, Class>::value)
    {
        metaClass = instance.getDynamicMetaClass();
    }

    auto visitor = [name = std::forward<std::string_view>(method), retType = MetaTypeDescriptor::typeId<Ret>()](const MetaMethod* method) -> bool
    {
        return (method->name() == name) && (method->returnType().type == retType);
    };
    const MetaMethod* metaMethod = metaClass->visitMethods(visitor);
    if (!metaMethod)
    {
        throw metamethod_not_found(method);
    }

    Callable::Arguments argPack(args...);
    if (metaMethod->type() == FunctionType::Method)
    {
        argPack.prepend(metaMethod->ownerClass().castInstance(&instance));
    }

    std::any result = metaMethod->apply(argPack);

    if constexpr (!std::is_void<Ret>::value)
    {
        return std::any_cast<Ret>(result);
    }
}

} // namespace mox

#endif // METAMETHOD_IMPL_HPP
