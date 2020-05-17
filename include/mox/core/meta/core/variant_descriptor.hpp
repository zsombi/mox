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

#include <mox/core/meta/core/metadata.hpp>

#include <vector>

namespace mox
{

/// Defines the type attributes of a Mox Variant.
struct MOX_API VariantDescriptor
{
    /// Constructor.
    VariantDescriptor() = default;
    VariantDescriptor(Metatype type, bool ref, bool c);

    /// Constructs a descriptor from a value.
    template <typename Type>
    VariantDescriptor(Type)
        : VariantDescriptor(metaType<Type>(), std::is_reference<Type>(), std::is_const<Type>())
    {
    }

    /// Returns the variant descriptor for the \e Type.
    /// \return The variant descriptor for the \e Type.
    template <typename Type>
    static VariantDescriptor get()
    {
        return VariantDescriptor(
                    metaType<Type>(),
                    std::is_reference<Type>(),
                    std::is_const<Type>()
                    );
    }

    Metatype getType() const
    {
        return static_cast<Metatype>(m_type);
    }
    bool isConst() const
    {
        return m_isConst;
    }
    bool isReference() const
    {
        return m_isReference;
    }

    /// Tests whether an \a other variant descriptor is compatible with this.
    bool invocableWith(const VariantDescriptor& other) const;

    /// Comparison operator, compares an \a other variant descriptor with this.
    bool operator==(const VariantDescriptor& other) const;

    /// Swaps variant descriptors.
    void swap(VariantDescriptor& other);

private:
    /// Tye metatype of the variant.
    Metatype m_type = Metatype::Invalid;
    /// \e true if the variant holds a reference value, \e false if not.
    bool m_isReference = false;
    /// \e true if the variant holds a const value, \e false if not.
    bool m_isConst = false;
};

/// Defines a container with variant descriptors.
class MOX_API VariantDescriptorContainer
{
    using BaseType = std::vector<VariantDescriptor>;
    BaseType m_container;

public:
    using iterator = BaseType::iterator;
    using const_iterator = BaseType::const_iterator;

    /// Construct the container from an other, using its iterators.
    template <typename Iterator>
    VariantDescriptorContainer(Iterator begin, Iterator end)
        : m_container(begin, end)
    {
    }

    /// Construct the container extracting the types from the parameters passed as arguments.
    template <typename... Arguments>
    VariantDescriptorContainer(Arguments...)
        : m_container(getArgs<Arguments...>().m_container)
    {
    }

    /// Fetch the variant descriptors from a set of argument types.
    template <typename... Arguments>
    static VariantDescriptorContainer getArgs()
    {
        const std::array<VariantDescriptor, sizeof... (Arguments)> aa = {{ VariantDescriptor::get<Arguments>()... }};
        return VariantDescriptorContainer(aa.begin(), aa.end());
    }

    /// Ensure the arguments are registered in the metatype system, and retrieve the
    /// container with the descriptors.
    template <typename... Arguments>
    static VariantDescriptorContainer ensure()
    {
        const std::array<Metatype, sizeof...(Arguments)> meta = {{registerMetaType<Arguments>()...}};
        UNUSED(meta);
        const std::array<VariantDescriptor, sizeof... (Arguments)> aa = {{ VariantDescriptor::get<Arguments>()... }};
        return VariantDescriptorContainer(aa.begin(), aa.end());
    }


    /// Tests whether the variant descriptors are compatible with the descriptors from
    /// the \a other container. A variant is compatible with an other if there is a
    /// callable with the argument as formal parameter, that is invocable with the other
    /// passed as actual parameter.
    /// \return If the variant types from \a other are compatible, returns \e true, otherwise \e false.
    bool isInvocableWith(const VariantDescriptorContainer& other) const;

    template <typename... Arguments>
    bool isInvocableWithArgumentTypes() const
    {
        return isInvocableWith(VariantDescriptorContainer::getArgs<Arguments...>());
    }

    void swap(VariantDescriptorContainer& other)
    {
        std::swap(m_container, other.m_container);
    }

    size_t size() const
    {
        return m_container.size();
    }
    bool empty() const
    {
        return m_container.empty();
    }
    void clear()
    {
        m_container.clear();
    }

    VariantDescriptor& operator[](size_t index)
    {
        return m_container[index];
    }
    const VariantDescriptor& operator[](size_t index) const
    {
        return m_container[index];
    }

    iterator begin()
    {
        return m_container.begin();
    }
    iterator end()
    {
        return m_container.end();
    }
    const_iterator begin() const
    {
        return m_container.begin();
    }
    const_iterator end() const
    {
        return m_container.end();
    }

    bool operator==(VariantDescriptorContainer& other) const
    {
        return m_container == other.m_container;
    }
    bool operator==(const VariantDescriptorContainer& other) const
    {
        return m_container == other.m_container;
    }
};

} // namespace mox

#endif // VARIANT_DESCRIPTOR_HPP
