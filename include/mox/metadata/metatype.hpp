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

#ifndef METATYPE_HPP
#define METATYPE_HPP

#include <any>

#include <mox/utils/globals.hpp>
#include <mox/utils/type_traits.hpp>

namespace mox
{

/// Defines the type identifier. User defined types are registered in the
/// user area, right after UserType.
enum class Metatype : int
{
    Invalid = -1,
    // void is a weirdo type
    Void = 0,
    Bool,
    Char,
    Byte,
    Short,
    Word,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    NumericMax = Double, // Identifies the maximum of the numeric types.
    //
    String,
    Literal,
    // Pointer types
    VoidPtr,
    BytePtr,
    // All user types to be installed here
    UserType
};
ENABLE_ENUM_OPERATORS(Metatype)

/// Base type of all Mox metavalues.
typedef std::any MetaValue;

/// Metatype converters
/// \{
/// Base converter.
struct MOX_API MetatypeConverter
{
    typedef MetaValue (*ConverterFunction)(const MetatypeConverter& /*converter*/, const void*/*value*/);

    /// Constructs a converter with a converter function.
    explicit MetatypeConverter(ConverterFunction function)
        : convert(function)
    {
    }
    explicit MetatypeConverter() = default;
    ConverterFunction convert = nullptr;

    DISABLE_COPY(MetatypeConverter)
};
typedef std::unique_ptr<MetatypeConverter> MetatypeConverterPtr;

/// \}

/// Exceptions
/// \{
/// Exception thrown when a type is not registered in the metadata.
class MOX_API type_not_registered : public std::exception
{
public:
    explicit type_not_registered(const std::type_info& rtti);
    const char* what() const EXCEPTION_NOEXCEPT override;
private:
    std::string m_message;
};

/// Exception thrown when a converter fails to convert a value from one metatype to other.
class MOX_API bad_conversion : public std::exception
{
public:
    explicit bad_conversion(Metatype from, Metatype to);
    const char* what() const EXCEPTION_NOEXCEPT override;
private:
    std::string m_message;
};

/// \}

} // mox

#endif // METATYPE_HPP
