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

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <mox/metadata/variant_descriptor.hpp>

namespace mox
{

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

    /// Returns \e true if this variant holds a valid value.
    bool isValid() const;

    /// Reset the variant.
    void reset();

    /// Returns the metatype of the value held by this variant.
    Metatype metaType() const;

    /// Returns the variant descriptor.
    const VariantDescriptor& descriptor() const;

private:
    struct Data
    {
        typedef std::function<void*()> GetterFunction;

        MetaValue m_value;
        mutable GetterFunction m_getter;
        VariantDescriptor m_typeDescriptor;

        template <typename T>
        explicit Data(T value);

        template <typename T>
        T get();
    };

    std::shared_ptr<Data> m_data;
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

} // namespace mox

#include <mox/metadata/detail/variant_impl.hpp>

#endif // ANY_HPP
