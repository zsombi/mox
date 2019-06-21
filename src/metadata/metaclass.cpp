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
    auto deriveTester = [&metaClass](const MetaClass& mc) -> MetaClass::VisitorResult
    {
        if (&mc == &metaClass)
        {
            return MetaClass::Abort;
        }
        return MetaClass::Continue;
    };
    // Visitor aborts if the metaclass is derived from a superclass.
    return visit(MetaClassVisitor(deriveTester)) == Abort;
}

const MetaClass* MetaClass::find(std::string_view className)
{
    return metadata().findMetaClass(className);
}

MetaClass::VisitorResult MetaClass::visit(const MetaClassVisitor &visitor) const
{
    if (visitor(*this) == Abort)
    {
        return Abort;
    }
    return visitSuperclasses(visitor);
}

MetaClass::VisitorResult MetaClass::visitSuperclasses(const MetaClassVisitor &visitor) const
{
    UNUSED(visitor);
    return Continue;
}

const MetaMethod* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    const MetaMethod* result = nullptr;

    auto tester = [&result, &visitor](const MetaClass& mc) -> VisitorResult
    {
        for (const MetaMethod* method : mc.m_methods)
        {
            if (visitor(method))
            {
                result = method;
                return Abort;
            }
        }
        return Continue;
    };

    visit(MetaClassVisitor(tester));
    return result;
}

const MetaSignal* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    const MetaSignal* result = nullptr;

    auto tester = [&result, &visitor](const MetaClass& mc) -> VisitorResult
    {
        for (const MetaSignal* signal : mc.m_signals)
        {
            if (visitor(signal))
            {
                result = signal;
                return Abort;
            }
        }
        return Continue;
    };

    visit(MetaClassVisitor(tester));
    return result;
}


void MetaClass::addMethod(MetaMethod *method)
{
    m_methods.push_back(method);
}

size_t MetaClass::addSignal(MetaSignal &signal)
{
    m_signals.push_back(&signal);
    return m_signals.size() - 1;
}

} // namespace mox
