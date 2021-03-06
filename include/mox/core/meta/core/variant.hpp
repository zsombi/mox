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

#ifndef ANY_HPP
#define ANY_HPP

#include <any>

#include <mox/core/meta/core/variant_descriptor.hpp>

namespace mox
{

struct VariantDescriptor;
/// The Variant class holds a value and its metatype passed as argument in metacalls.
struct MOX_API Variant
{
    /// Constructor.
    explicit Variant() = default;

    /// Templated constructor, initializes the argument with a given value.
    template <typename T>
    explicit Variant(const T value);

    /// Copy constructor.
    Variant(const Variant& other);
    /// Move constructor.
    Variant(Variant&& other);

    /// Chacks if this variant is convertible into type T.
    /// \return \e true if this variant is convertible into type T, \e false otherwise.
    template <typename T>
    bool canConvert();

    /// Cast operator, returns the value stored by this variant, or the value converted
    /// in the desired type.
    /// \throws bad_conversion if this variant is not convertible to the requested type.
    template <typename T>
    operator T();

    /// Const cast operator, returns the value stored by this variant, or the value converted
    /// in the desired type.
    /// \throws bad_conversion if this variant is not convertible to the requested type.
    template <typename T>
    operator T() const;

    /// Assignment operator.
    template <typename T>
    Variant& operator=(const T value);
    /// Copy assignment operator.
    Variant& operator=(const Variant&);
    /// Move assignment operator.
    Variant& operator=(Variant&&);

    /// Returns \e true if this variant holds a valid value.
    bool isValid() const;

    /// Reset the variant.
    void reset();

    /// Returns the metatype of the value held by this variant.
    Metatype metaType() const;

    /// Returns the variant descriptor.
    const VariantDescriptor& descriptor() const;

    /// Swaps variants.
    void swap(Variant& other);

private:
    struct Data
    {
        typedef std::function<void*()> GetterFunction;
        typedef std::function<bool(const Data&)> EqualFunction;

        MetaValue m_value;
        mutable GetterFunction m_getter;
        mutable EqualFunction m_isEqual;
        VariantDescriptor m_typeDescriptor;

        template <typename T>
        explicit Data(T value);

        template <typename T>
        T get();
    };

    std::shared_ptr<Data> m_data;
    friend bool operator==(const Variant &var1, const Variant &var2);
};

template <typename T>
bool operator==(T const& value, const Variant& arg)
{
    const T v = arg;
    return value == v;
}

template <typename T>
bool operator==(const Variant& arg, T const& value)
{
    const T v = arg;
    return value == v;
}

template <typename T>
bool operator!=(T const& value, const Variant& arg)
{
    const T v = arg;
    return value != v;
}

template <typename T>
bool operator!=(const Variant& arg, T const& value)
{
    const T v = arg;
    return value != v;
}

template <typename T>
T operator%(const Variant& arg, T const& value)
{
    T v = arg;
    return v % value;
}

template <typename T>
T operator%(T const& value, const Variant& arg)
{
    T v = arg;
    return value % v;
}

/// Compares two variants' equality. The variants must have the same type and value. No conversion is involved.
bool MOX_API operator==(const Variant &var1, const Variant &var2);

/// Compares two variants' inequality. No conversion is involved.
bool MOX_API operator!=(const Variant &var1, const Variant &var2);

} // namespace mox

namespace std
{

void swap(mox::Variant& lhs, mox::Variant& rhs);

}

#include <mox/core/meta/core/detail/variant_impl.hpp>

#endif // ANY_HPP
