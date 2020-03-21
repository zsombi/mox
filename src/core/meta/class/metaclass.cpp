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

#include <mox/core/meta/class/metaclass.hpp>
#include <metadata_p.hpp>
#include <signal_p.hpp>
#include <mox/core/object.hpp>

namespace mox
{
namespace metainfo
{

AbstractMetaInfo::AbstractMetaInfo(std::string_view name)
    : m_name(name)
{
}

std::string AbstractMetaInfo::name() const
{
    return m_name;
}

/******************************************************************************
 * MetaClass::MetaSignalBase
 */
MetaClass::MetaSignalBase::MetaSignalBase(MetaClass& hostClass, VariantDescriptorContainer&& args, std::string_view name)
    : SignalType(std::forward<VariantDescriptorContainer>(args))
    , AbstractMetaInfo(name)
{
    hostClass.addMetaSignal(*this);
}

std::string MetaClass::MetaSignalBase::signature() const
{
    std::string sign = name() + '(';
    for (auto& des : m_argumentDescriptors)
    {
        sign += MetatypeDescriptor::get(des.getType()).name();
        sign += ',';
    }
    if (!m_argumentDescriptors.empty())
    {
        sign.back() = ')';
    }
    else
    {
        sign += ')';
    }
    return sign;
}

/******************************************************************************
 * MetaClass::MetaPropertyType
 */
MetaClass::MetaPropertyBase::MetaPropertyBase(MetaClass& hostClass, VariantDescriptor&& typeDes, PropertyAccess access, const MetaSignalBase& signal, PropertyDataProviderInterface& defaultValue, std::string_view name)
    : PropertyType(std::forward<VariantDescriptor>(typeDes), access, signal, defaultValue)
    , AbstractMetaInfo(name)
{
    hostClass.addMetaProperty(*this);
}

std::string MetaClass::MetaPropertyBase::signature() const
{
    return name() + '<' + MetatypeDescriptor::get(m_typeDescriptor.getType()).name() + '>';
}

/******************************************************************************
 * MetaClass
 */
std::string MetaClass::MetaMethodBase::signature() const
{
    std::string sign(MetatypeDescriptor::get(returnType().getType()).name());
    sign += ' ' + name() + '(';
    for (auto& des : descriptors())
    {
        sign += MetatypeDescriptor::get(des.getType()).name();
        sign += ',';
    }
    if (!descriptors().empty())
    {
        sign.back() = ')';
    }
    else
    {
        sign += ')';
    }
    return sign;
}
/******************************************************************************
 * MetaClass
 */
void MetaClass::addMetaMethod(Callable& method)
{
    auto metaMethod = dynamic_cast<MetaMethodBase*>(&method);
    FATAL(metaMethod, "You can only add MetaMethods to a MetaClass.");
    m_metaMethods.push_back(metaMethod);
}

void MetaClass::addMetaSignal(SignalType& signal)
{
    auto metaSignal = dynamic_cast<MetaSignalBase*>(&signal);
    FATAL(metaSignal, "You can only add MetaSignals to a MetaClass.");
    m_metaSignals.push_back(metaSignal);
}

void MetaClass::addMetaProperty(PropertyType& property)
{
    auto metaProperty = dynamic_cast<MetaPropertyBase*>(&property);
    FATAL(metaProperty, "You can only add MetaProperty to a MetaClass.");
    m_metaProperties.push_back(metaProperty);
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

const Callable* MetaClass::visitMethods(const MethodVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto method : mc.m_metaMethods)
        {
            if (visitor(method, *method))
            {
                return std::make_tuple(Abort, method);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };

    VisitorResultType result = visit(MetaClassVisitor(tester));
    MetaValue method = std::get<1>(result);
    return (std::get<0>(result) == Abort)
            ? static_cast<const Callable*>(std::any_cast<const MetaMethodBase*>(method))
            : nullptr;
}

const SignalType* MetaClass::visitSignals(const SignalVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (auto signal : mc.m_metaSignals)
        {
            if (visitor(signal, *signal))
            {
                return std::make_tuple(Abort, signal);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };

    auto result = visit(MetaClassVisitor(tester));
    auto signal = std::get<1>(result);
    return (std::get<0>(result) == Abort)
            ? static_cast<const SignalType*>(std::any_cast<const MetaSignalBase*>(signal))
            : nullptr;
}

const PropertyType* MetaClass::visitProperties(const PropertyVisitor& visitor) const
{
    auto tester = [&visitor](const MetaClass& mc) -> VisitorResultType
    {
        for (const auto property : mc.m_metaProperties)
        {
            if (visitor(property, *property))
            {
                return std::make_tuple(Abort, property);
            }
        }
        return std::make_tuple(Continue, MetaValue());
    };
    auto result = visit(tester);
    auto property = std::get<1>(result);
    return (std::get<0>(result) == Abort)
            ? static_cast<const PropertyType*>(std::any_cast<const MetaPropertyBase*>(property))
            : nullptr;
}

/******************************************************************************
 * meta
 */
Signal::ConnectionSharedPtr connect(Signal& signal, MetaBase& receiver, const Callable& metaMethod)
{
    Object* recv = dynamic_cast<Object*>(&receiver);
    if (recv)
    {
        return Signal::Connection::create<ObjectMetaMethodConnection>(signal, *recv, metaMethod);
    }
    else
    {
        return Signal::Connection::create<MetaMethodConnection>(signal, Variant(&receiver), metaMethod);
    }
}

} // metainfo

} // namespace mox
