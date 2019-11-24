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

std::string MetaClass::Method::name() const
{
    return m_name;
}


MetaClass::Signal::Signal(MetaClass& metaClass, const SignalType& type, std::string_view name)
    : m_ownerClass(metaClass)
    , m_type(type)
    , m_name(name)
{
    m_ownerClass.addSignal(*this);
}

std::string MetaClass::Signal::name() const
{
    return m_name;
}

const SignalType& MetaClass::Signal::type() const
{
    return m_type;
}

int MetaClass::Signal::activate(intptr_t sender, const Callable::ArgumentPack &arguments) const
{
    return m_type.activate(sender, arguments);
}

bool MetaClass::Signal::isInvocableWith(const VariantDescriptorContainer &arguments) const
{
    return m_type.getArguments().isInvocableWith(arguments);
}

void MetaClass::addMethod(const Method &method)
{
    m_methods.push_back(&method);
}

void MetaClass::addSignal(const Signal &signal)
{
    m_signals.push_back(&signal);
}

void MetaClass::addProperty(const Property &property)
{
    m_properties.push_back(&property);
}

MetaClass::MetaClass(const MetatypeDescriptor& type)
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

const MetaClass::Method* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto method : mc.m_methods)
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
    return (std::get<0>(result) == Abort) ? std::any_cast<const MetaClass::Method*>(method) : nullptr;
}

const MetaClass::Signal* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto signal : mc.m_signals)
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
    return (std::get<0>(result) == Abort) ? std::any_cast<const MetaClass::Signal*>(signal) : nullptr;
}

const MetaClass::Property* MetaClass::visitProperties(const PropertyVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (const auto property : mc.m_properties)
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
    return (std::get<0>(result) == Abort) ? std::any_cast<const MetaClass::Property*>(property) : nullptr;
}

} // namespace mox
