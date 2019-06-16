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

MetaClass::MetaClass(const MetatypeDescriptor& type, bool abstract)
    : m_type(type)
    , m_isAbstract(abstract)
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
    auto tester = [&metaClass] (const MetaClass* mc) -> bool
    {
        if (mc == &metaClass)
        {
            return true;
        }
        else if (mc->derivesFrom(metaClass))
        {
            return true;
        }
        return false;
    };

    MetaClassContainer::const_iterator it = std::find_if(m_superClasses.cbegin(), m_superClasses.cend(), tester);
    return (it != m_superClasses.cend());
}

const MetaClass* MetaClass::find(std::string_view className)
{
    return metadata().findMetaClass(className);
}

const MetaMethod* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    for (const MetaMethod* method : m_methods)
    {
        if (visitor(method))
        {
            return method;
        }
    }

    for (const MetaClass* super : m_superClasses)
    {
        const MetaMethod* method = super->visitMethods(visitor);
        if (method)
        {
            return method;
        }
    }

    return nullptr;
}

const MetaSignal* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    for (const MetaSignal* signal : m_signals)
    {
        if (visitor(signal))
        {
            return signal;
        }
    }

    for (const MetaClass* super : m_superClasses)
    {
        const MetaSignal* signal = super->visitSignals(visitor);
        if (signal)
        {
            return signal;
        }
    }

    return nullptr;
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
