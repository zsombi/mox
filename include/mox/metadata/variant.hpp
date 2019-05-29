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

#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

#include <variant>
#include <string>

namespace mox
{

class MetaObject;
class Variant;

namespace
{
// Base type for Mox Variant. The order of the variant types matches the MetaTypeDescriptor::TypeId order.
typedef std::variant<
    std::monostate,
    bool,
    char,
    byte,
    short,
    unsigned short,
    int,
    unsigned int,
    long,
    unsigned long,
    long long,
    unsigned long long,
    float,
    double,
    std::string,
    MetaObject*,
    void*> VariantBase;

}

/// Mox Variant class, holds an object of a heterogenous set of types.
class MOX_API Variant : public VariantBase
{
public:
    /// Constructs an invalid variant.
    explicit Variant();

    /// Constructs a variant with a value of a given type. You can set only types supported by the
    /// variant. To set custom types, use the assignment operator.
    /// \param value The initial value of the variant.
    template<class T>
    explicit Variant(const T& value)
        : VariantBase(value)
    {        
    }

    /// Assigns a value of a given type to the variant.
    template <class T>
    Variant& operator=(const T& value)
    {
        VariantBase::operator=(value);
        return *this;
    }

    /// Copy constructor.
    Variant(const Variant& other);

    /// Assignemt operator, accepting an \a other variant.
    Variant& operator=(Variant other);

    /// Explicit bool operator used to test the validity of the variant.
    explicit operator bool() const;

    /// Returns the metatype of the variant.
    MetaTypeDescriptor::TypeId type() const;

    /// Returns \e true if the current value of the variant is of type \a Type.
    /// \return \e true if the current value of the variant is of \a Type, \e false if not.
    template <typename Type>
    bool isValueType() const
    {
        return MetaTypeDescriptor::typeId<Type>() == type();
    }

    /// Returns the value managed by the variant. Throws std::bad_variant_access if the value managed by the
    /// variant is not of the requested type.
    template <class T>
    T value() const
    {
        return std::get<T>(*this);
    }

    /// Returns true if the variant data is convertible to the desired \a toType.
    /// \param toType The desired type to convert the variant data.
    /// \return \e true if the variant is convertible to the type, \e false if not.
    bool canConvert(MetaTypeDescriptor::TypeId toType) const;
};

/// Comparison operator, compares two variants.
bool operator==(const Variant& v1, const Variant& v2);

/// Comparison operator, compares a value of a given type with a variant.
template <class T>
bool operator==(const T& v, const Variant& var)
{
    mox::Variant v1(v);
    return v1 == var;
}

/// Comparison operator, compares a value of a given type with a variant.
template <class T>
bool operator==(const Variant& var, const T& v)
{
    mox::Variant v1(v);
    return v1 == var;
}

/// Converts a \a variant into an other one with the desired \a Type. The function throws \e std::bad_variant_access
/// if there is no converter registered for the type held by the variant.
/// \param variant The variant to convert.
/// \return The value converted from the variant. If the conversion fails, an invalid value is returned.
template <typename Type>
Type variant_cast(Variant variant)
{
    MetaTypeDescriptor::TypeId toType = MetaTypeDescriptor::typeId<Type>();
    if (variant.type() == toType)
    {
        // No conversion needed.
        return variant.value<Type>();
    }

    MetaTypeDescriptor::AbstractConverterSharedPtr converter = MetaTypeDescriptor::findConverter(variant.type(), toType);
    if (!converter)
    {
        throw std::bad_variant_access();
    }

    Type result;
    auto converterVisitor = [converter, &result] (auto&& value)
    {
        converter->m_convert(*converter, reinterpret_cast<const void*>(&value), reinterpret_cast<void*>(&result));
    };

    std::visit(converterVisitor, variant);

    return result;
}

} // namespace mox

#endif // VARIANT_HPP
