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
#include <mox/metadata/metamethod.hpp>
#include "metadata_p.h"

namespace mox
{

MetaClass::MetaClass(const MetatypeDescriptor& type)
    : m_type(type)
{
    UNUSED(__padding);
    metadata().addMetaClass(*this);
}

MetaClass::~MetaClass()
{
    metadata().removeMetaClass(*this);
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
        return std::make_tuple(MetaClass::Continue, std::any());
    };
    // Visitor aborts if the metaclass is derived from a superclass.
    return std::get<0>(visit(MetaClassVisitor(deriveTester))) == Abort;
}

const MetaClass* MetaClass::find(std::string_view className)
{
    return metadata().findMetaClass(className);
}

MetaClass::VisitorResultType MetaClass::visit(const MetaClassVisitor &visitor) const
{
    VisitorResultType result = visitor(*this);
    if (std::get<0>(result) == Abort)
    {
        return result;
    }
    return visitSuperclasses(visitor);
}

MetaClass::VisitorResultType MetaClass::visitSuperclasses(const MetaClassVisitor &visitor) const
{
    UNUSED(visitor);
    return std::make_tuple(Continue, std::any());
}

const MetaMethod* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (const MetaMethod* method : mc.m_methods)
        {
            if (visitor(method))
            {
                return std::make_tuple(Abort, method);
            }
        }
        return std::make_tuple(Continue, std::any());
    };

    VisitorResultType result = visit(MetaClassVisitor(tester));
    std::any method = std::get<1>(result);
    return (std::get<0>(result) == Abort) ? std::any_cast<const MetaMethod*>(method) : nullptr;
}

const MetaSignal* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (const MetaSignal* signal : mc.m_signals)
        {
            if (visitor(signal))
            {
                return std::make_tuple(Abort, signal);
            }
        }
        return std::make_tuple(Continue, std::any());
    };

    VisitorResultType result = visit(MetaClassVisitor(tester));
    std::any signal = std::get<1>(result);
    return (std::get<0>(result) == Abort) ? std::any_cast<const MetaSignal*>(signal) : nullptr;
}


void MetaClass::addMethod(MetaMethod *method)
{
    m_methods.push_back(method);
}

size_t MetaClass::addSignal(MetaSignal &signal)
{
    m_signals.push_back(&signal);

    // Get the next signal ID.
    size_t id = 0;
    auto looper = [&id](const MetaClass& metaClass) -> VisitorResultType
    {
        id += metaClass.m_signals.size();
        return make_tuple(Continue, std::any());
    };
    visit(looper);
    return id - 1;
}

} // namespace mox
