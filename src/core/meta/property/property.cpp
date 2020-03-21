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

#include <signal_p.hpp>
#include <property_p.hpp>
#include <mox/core/meta/property/property.hpp>
#include <mox/config/error.hpp>
#include <mox/core/meta/property/binding/binding.hpp>

#include <binding_p.hpp>
#include <metabase_p.hpp>

namespace mox
{

void PropertyDataProvider::update(const Variant& newValue)
{
    FATAL(m_property, "The property data provider is not attached to a property");
    m_property->updateData(newValue);
}

/******************************************************************************
 * Property - public API
 */
Property::Property(MetaBase& host, const PropertyType& type, PropertyDataProvider& data)
    : SharedLock(host)
    , changed(host, type.ChangedSignalType)
    , d_ptr(pimpl::make_d_ptr<PropertyStorage>(*this, host, type, data))
{
}

Property::~Property()
{
    if (d_ptr)
    {
        d_ptr->destroy();
    }
}

bool Property::isValid() const
{
    return d_ptr != nullptr;
}

bool Property::isReadOnly() const
{
    return isValid() && (d_ptr->getType().getAccess() == PropertyAccess::ReadOnly);
}

Variant Property::get() const
{
    throwIf<ExceptionType::InvalidProperty>(!isValid());
    lock_guard lock(const_cast<Property&>(*this));
    const_cast<PropertyStorage*>(d_ptr.get())->notifyAccessed();
    return d_ptr->fetchDataUnsafe();
}

void Property::set(const Variant& value)
{
    throwIf<ExceptionType::InvalidProperty>(!isValid());
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());

    // Detach bindings that are not permanent.
    d_ptr->detachNonPermanentBindings();

    // Set the value.
    d_ptr->updateData(value);
}

void Property::reset()
{
    throwIf<ExceptionType::InvalidProperty>(!isValid());
    throwIf<ExceptionType::AttempWriteReadOnlyProperty>(isReadOnly());
    d_ptr->resetToDefault();
}

BindingSharedPtr Property::getCurrentBinding()
{
    throwIf<ExceptionType::InvalidProperty>(!isValid());
    lock_guard lock(*this);
    return d_ptr->getTopBinding();
}

/******************************************************************************
 * DynamicProperty
 */
DynamicProperty::DynamicProperty(MetaBase& host, const PropertyType& type)
    : Property(host, type, *this)
{
}

DynamicPropertyPtr DynamicProperty::create(MetaBase &host, const PropertyType &type)
{
    auto property = DynamicPropertyPtr(new DynamicProperty(host, type));
    MetaBasePrivate::get(host)->addDynamicProperty(property);
    return property;
}

}
