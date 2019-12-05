/*
 * Copyright (C) 2017-2018 bitWelder
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

#include <mox/metadata/metaclass.hpp>
#include "metadata_p.h"
#include <mox/signal/signal.hpp>

namespace mox
{

void MetaClass::addMetaMethod(MethodType& method)
{
    m_metaMethods.push_back(&method);
}

void MetaClass::addMetaSignal(SignalType &signal)
{
    m_metaSignals.push_back(&signal);
}

void MetaClass::addMetaProperty(PropertyType& property)
{
    m_metaProperties.push_back(&property);
}

MetaClass::MetaClass(std::pair<Metatype, Metatype> type)
    : m_type(type)
{
    MetaData::addMetaClass(*this);
}

MetaClass::~MetaClass()
{
    MetaData::removeMetaClass(*this);
}

bool MetaClass::isSuperClassOf(const MetaClass &metaClass) const
{
    return metaClass.derivesFrom(*this);
}

bool MetaClass::derivesFrom(const MetaClass &metaClass) const
{
    auto deriveTester = [&metaClass](const MetaClass& mc) -> VisitorResultType
    {
        if (&mc == &metaClass)
        {
            return std::make_tuple(MetaClass::Abort, &mc);
        }
        return std::make_tuple(MetaClass::Continue, MetaValue());
    };
    // Visitor aborts if the metaclass is derived from a superclass.
    return std::get<0>(visit(MetaClassVisitor(deriveTester))) == Abort;
}

const MetaClass* MetaClass::find(std::string_view className)
{
    return MetaData::findMetaClass(className);
}

MetaClass::VisitorResultType MetaClass::visit(const MetaClassVisitor &visitor) const
{
    VisitorResultType result = visitor(*this);
    if (std::get<0>(result) == Abort)
    {
        return result;
    }
    return visitSuperClasses(visitor);
}

MetaClass::VisitorResultType MetaClass::visitSuperClasses(const MetaClassVisitor &visitor) const
{
    UNUSED(visitor);
    return std::make_tuple(Continue, MetaValue());
}

const MethodType* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto method : mc.m_metaMethods)
        {
            if (visitor(method))
            {
                return std::make_tuple(Abort, method);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };

    VisitorResultType result = visit(MetaClassVisitor(tester));
    MetaValue method = std::get<1>(result);
    return (std::get<0>(result) == Abort) ? std::any_cast<const MethodType*>(method) : nullptr;
}

const SignalType* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto signal : mc.m_metaSignals)
        {
            if (visitor(signal))
            {
                return std::make_tuple(Abort, signal);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };

    auto result = visit(MetaClassVisitor(tester));
    auto signal = std::get<1>(result);
    return (std::get<0>(result) == Abort) ? std::any_cast<const SignalType*>(signal) : nullptr;
}

const PropertyType* MetaClass::visitProperties(const PropertyVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (const auto property : mc.m_metaProperties)
        {
            if (visitor(property))
            {
                return std::make_tuple(Abort, property);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };
    auto result = visit(tester);
    auto property = std::get<1>(result);
    return (std::get<0>(result) == Abort) ? std::any_cast<const PropertyType*>(property) : nullptr;
}

} // namespace mox
