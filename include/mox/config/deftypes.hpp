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

#ifndef DEFTYPES_HPP
#define DEFTYPES_HPP

#include <mox/config/platform_config.hpp>
// Standard integer types
#include <inttypes.h>
#include <chrono>
#include <string>
#include <string_view>

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

using byte = int8_t;
using long_t = long int;
using ulong_t = unsigned long int;


#ifdef ANDROID

typedef long intptr_t_;

#endif

namespace mox
{

using Timestamp = std::chrono::system_clock::time_point;

typedef int64_t TUuid;


/// Base class for types with meta information.
class MOX_API AbstractMetaInfo
{
public:
    virtual ~AbstractMetaInfo() = default;

    /// Returns the name of the metainfo.
    /// \return The name of the metainfo.
    std::string name() const;

    /// Returns the signature of the metainfo.
    virtual std::string signature() const = 0;

protected:
    /// Constructor.
    explicit AbstractMetaInfo(std::string_view name);

private:
    std::string m_name;
};

/// Class instance holder, used to store and pass class pointers without preserving type information.
class MOX_API Instance
{
    intptr_t m_instance = 0;

    Instance(intptr_t instance);

public:
    /// Default constructor.
    Instance() = default;
    /// Copy constructor.
    Instance(const Instance& other) = default;
    /// Move constructor.
    Instance(Instance&& other) noexcept;

    /// Creates an instance holder from a class pointer.
    /// \tparam Class The class type.
    /// \param instance The pointer to the class instance.
    template <class Class>
    Instance(Class* instance)
        : Instance(intptr_t(instance))
    {
    }

    /// Cast operator.
    operator intptr_t() const;

    /// Cast the instance as a type.
    /// \tparam Type The type to convert to.
    /// \return The pointer of Type the instance is converted.
    template <typename Type>
    Type* as() const noexcept
    {
        return reinterpret_cast<Type*>(m_instance);
    }

    /// Reset the instance.
    void reset() noexcept;

    /// Equality comparison operator.
    bool operator==(const Instance& rhs);

    /// Assignment operator.
    Instance& operator=(const Instance& rhs);
};

/// Compare an Instance with an intptr_t.
bool operator==(const Instance& lhs, intptr_t rhs);

/// Compare an intptr_t with an Instance.
bool operator==(intptr_t lhs, const Instance& rhs);

}

#endif // DEFTYPES_HPP
