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

#ifndef PROPERTY_TYPE_HPP
#define PROPERTY_TYPE_HPP

#include <mox/config/deftypes.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/utils/flat_map.hpp>
#include <mox/metadata/variant.hpp>
#include <mox/signal/signal_type.hpp>
#include <mox/metadata/metaclass.hpp>

namespace mox
{

class Property;

/// PropertyAccess defines the access types of a property in Mox.
enum class PropertyAccess
{
    /// Identifies a read-only property.
    ReadOnly,
    /// Identifies a read-write property.
    ReadWrite
};

/// PropertyType declares the type identification of a property. Holds the metatype of the
/// property data, the change signal type and the instances of the property.
class MOX_API PropertyType : public ObjectLock, public AbstractMetaInfo
{
public:
    /// Destructor.
    ~PropertyType();

    /// Returns the change signal type of the property. Both read-only and read-write properties
    /// have change signals.
    /// \return The reference to the signal type.
    virtual SignalType& getChangedSignalType() = 0;

    /// Returns the access type of the property.
    /// \return The access type of the property.
    /// \see PropertyAccess
    PropertyAccess getAccess() const;

    /// Returns the type of the property.
    const VariantDescriptor& getValueType() const;

    /// Adds the instance of the property to track.
    void addPropertyInstance(Property& property);
    /// Removes the instance of the property.
    void removePropertyInstance(Property& property);

    /// Get the property value that is declared on an \a instance.
    /// \param instance The instance thr property is declared on.
    /// \return The variant with the property value. If the property is not defined on the
    /// instance, returns an invalid variant.
    Variant get(Instance instance) const;
    /// Set the value of the property declared on an \a instance.
    /// \param instance The instane the property is declared.
    /// \param value The variant with the value to set.
    /// \return If the property is not declared on the instance, or the value is not valid for
    /// the property, returns \e false, otherwise \e true.
    bool set(Instance instance, const Variant& value) const;

    /// Returns the signature of the property.
    std::string signature() const override;

protected:
    /// Constructor, creates a property type.
    /// \param typeDes The variant type descriptor of the type.
    /// \param signalType The reference to the signal type defining the change signal of the property.
    /// \param access The access type of the property.
    PropertyType(VariantDescriptor&& typeDes, PropertyAccess access, std::string_view name);

    using InstanceCollection = FlatMap<intptr_t, Property*>;

    /// Map of the property instances.
    InstanceCollection m_instances;
    /// The type descriptor of the property.
    VariantDescriptor m_typeDescriptor;
    /// The property access type.
    PropertyAccess m_access = PropertyAccess::ReadWrite;
};

/// Property declarator template. Use this to define the property types.
/// \tparam ValueType The value type of the property.
/// \tparam access The access value of the property.
template <class HostClass, typename ValueType, PropertyAccess access>
class PropertyTypeDecl : public PropertyType
{
    DISABLE_COPY(PropertyTypeDecl)

public:
    /// The change signal type.
    SignalTypeDecl<HostClass, ValueType> ChangedSignalType;

    /// Constructor.
    explicit PropertyTypeDecl(std::string_view name)
        : PropertyType(VariantDescriptor::get<ValueType>(), access, name)
        , ChangedSignalType(std::string(name) + "Changed")
    {
        if constexpr (has_static_metaclass_v<HostClass>)
        {
            registerMetaClass<HostClass>();
            HostClass::__getStaticMetaClass()->addMetaProperty(*this);
        }
    }

    SignalType & getChangedSignalType() override
    {
        return ChangedSignalType;
    }
};

template <typename ValueType, class Class>
std::pair<ValueType, bool> property(Class& instance, std::string_view name)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    auto finder = [&name](const PropertyType* property)
    {
        return property->name() == name;
    };
    auto property = metaClass->visitProperties(finder);
    if (property)
    {
        ValueType value = property->get(&instance);
        return std::make_pair(value, true);
    }
    return std::make_pair(ValueType(), false);
}

template <typename ValueType, class Class>
bool setProperty(Class& instance, std::string_view name, ValueType value)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    auto finder = [&name](const PropertyType* property)
    {
        return property->name() == name;
    };
    auto property = metaClass->visitProperties(finder);
    if (property)
    {
        return property->set(&instance, Variant(value));
    }
    return false;
}

} //namespace mox

#endif // PROPERTY_TYPE_HPP
