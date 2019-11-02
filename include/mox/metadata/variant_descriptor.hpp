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

#ifndef VARIANT_DESCRIPTOR_HPP
#define VARIANT_DESCRIPTOR_HPP

#include <mox/metadata/metadata.hpp>

#include <vector>

namespace mox
{

/// Defines the type attributes of a Mox Variant.
struct MOX_API VariantDescriptor
{
    /// Tye metatype of the variant.
    const Metatype type = Metatype::Invalid;
    /// \e true if the variant holds a reference value, \e false if not.
    const bool isReference = false;
    /// \e true if the variant holds a const value, \e false if not.
    const bool isConst = false;

    /// Constructor.
    VariantDescriptor() = default;
    VariantDescriptor(Metatype type, bool ref, bool c);

    /// Constructs a descriptor from a value.
    template <typename Type>
    VariantDescriptor(Type)
        : type(metaType<Type>())
        , isReference(std::is_reference<Type>())
        , isConst(std::is_const<Type>())
    {
    }

    /// Returns the variant descriptor for the \e Type.
    /// \return The variant descriptor for the \e Type.
    template <typename Type>
    static VariantDescriptor&& get()
    {
        return std::move(VariantDescriptor{
                             metaType<Type>(),
                             std::is_reference<Type>(),
                             std::is_const<Type>()
                         });
    }

    /// Tests whether an \a other variant descriptor is compatible with this.
    bool invocableWith(const VariantDescriptor& other) const;

    /// Comparison operator, compares an \a other variant descriptor with this.
    bool operator==(const VariantDescriptor& other) const;

    /// Swaps variant descriptors.
    void swap(VariantDescriptor& other);
};

/// Defines a container with variant descriptors.
class MOX_API VariantDescriptorContainer : public std::vector<VariantDescriptor>
{
    typedef std::vector<VariantDescriptor> BaseType;
public:

    /// Construct the container from an other, using its iterators.
    template <typename Iterator>
    VariantDescriptorContainer(Iterator begin, Iterator end)
        : BaseType(begin, end)
    {
    }

    /// Construct the container extracting the types from the parameters passed as arguments.
    template <typename... Arguments>
    VariantDescriptorContainer(Arguments...)
        : BaseType(get<Arguments...>())
    {
    }

    /// Fetch the variant descriptors from a set of argument types.
    template <typename... Arguments>
    static VariantDescriptorContainer get()
    {
        const std::array<VariantDescriptor, sizeof... (Arguments)> aa = {{ VariantDescriptor::get<Arguments>()... }};
        return VariantDescriptorContainer(aa.begin(), aa.end());
    }

    /// Tests whether the variant descriptors are compatible with the descriptors from
    /// the \a other container. A variant is compatible with an other if there is a
    /// callable with the argument as formal parameter, that is invocable with the other
    /// passed as actual parameter.
    /// \return If the variant types from \a other are compatible, returns \e true, otherwise \e false.
    bool isInvocableWith(const VariantDescriptorContainer& other) const;
};

} // namespace mox

#endif // VARIANT_DESCRIPTOR_HPP
