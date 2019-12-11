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

/******************************************************************************
 * StaticMetaClass
 */
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::StaticMetaClass()
    : mox::MetaClass({mox::metaType<BaseClass>(), mox::metaType<BaseClass*>()})
{
    std::array<bool, 2 * sizeof...(SuperClasses)> casters =
    {{
        MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<BaseClass*, SuperClasses*>(), getMetaTypes().second, mox::registerClassMetaTypes<SuperClasses>().second)...,
        MetatypeDescriptor::registerConverter(MetatypeDescriptor::Converter::dynamicCast<SuperClasses*, BaseClass*>(), mox::metaType<SuperClasses*>(), getMetaTypes().second)...
    }};
    UNUSED(casters);
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

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
const typename StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::DeclaredMetaClass* StaticMetaClass<MetaClassDecl, BaseClass, SuperClasses...>::get()
{
    return BaseClass::__getStaticMetaClass();
}

/******************************************************************************
 * helpers
 */
template <class ClassType>
std::pair<Metatype, Metatype> registerMetaClass(std::string_view name)
{
    auto typePair = registerClassMetaTypes<ClassType>(name);
    ClassType::StaticMetaClass::get();
    return typePair;
}

} // namespace mox

#endif // METACLASS_IMPL_HPP
