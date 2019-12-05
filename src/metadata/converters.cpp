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

#include <mox/config/platform_config.hpp>
#include <mox/config/deftypes.hpp>
#include <mox/metadata/metatype.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <string>
#include "metadata_p.h"
#include <cxxabi.h>

#include <mox/utils/function_traits.hpp>
#include <mox/utils/string.hpp>

#include <sstream>

namespace mox
{

inline bool isNumericMetatype(Metatype type)
{
    return type >= Metatype::Bool && type <= Metatype::Double;
}


template <typename From, typename To>
To atomicConverter(From value)
{
    return static_cast<To>(value);
}

template <typename From, typename To, typename Function>
void internal_registerConverter(Function converter)
{
    MetatypeDescriptor* fromType = metadata::findMetatypeDescriptor(remove_cv<From>());
    Metatype to = metadata::findMetatypeDescriptor(remove_cv<To>())->id();
    fromType->addConverter(std::make_unique<ConverterFunctor<From, To, Function>>(converter), to);
}

template <typename From, typename To>
void registerAtomicConverter()
{
    internal_registerConverter<From, To>(atomicConverter<From, To>);
    internal_registerConverter<To, From>(atomicConverter<To, From>);
}

bool stringToBool(std::string value)
{
    return (string_tolower(value) == "true");
}

bool literalToBool(std::string_view value)
{
    return stringToBool(value.data());
}

std::string boolToString(bool value)
{
    return value ? "true" : "false";
}

std::istringstream& operator>>(std::istringstream& is, byte& value)
{
    unsigned char _byte;
    is >> _byte;
    value = static_cast<byte>(_byte);
    return is;
}

template <typename String, typename To>
To stringToNumber(String value)
{
    To result = To();
    if (!value.empty())
    {
        std::stringstream istream(value.data());
        if (value.find("0x") != std::string::npos)
        {
            istream >> std::hex;
            istream >> result;
        }
        else
        {
            istream >> std::dec;
            istream >> result;
        }
        throwIf<ExceptionType::BadTypeConversion>(istream.fail());
    }
    return result;
}

template <typename From, typename String>
String numberToString(From from)
{
    std::ostringstream os;
    os << from;
    return os.str();
}

template <typename T>
void registerStringConverter()
{
    internal_registerConverter<std::string, T>(stringToNumber<std::string, T>);
    internal_registerConverter<T, std::string>(numberToString<T, std::string>);
    internal_registerConverter<std::string_view, T>(stringToNumber<std::string_view, T>);
}

std::string literalToString(std::string_view value)
{
    return value.data();
}

// Registrar function
void registerConverters()
{
    // bool
    registerAtomicConverter<bool, char>();
    registerAtomicConverter<bool, byte>();
    registerAtomicConverter<bool, short>();
    registerAtomicConverter<bool, unsigned short>();
    registerAtomicConverter<bool, int32_t>();
    registerAtomicConverter<bool, uint32_t>();
    registerAtomicConverter<bool, int64_t>();
    registerAtomicConverter<bool, uint64_t>();
    registerAtomicConverter<bool, float>();
    registerAtomicConverter<bool, double>();
    // char
    registerAtomicConverter<char, byte>();
    registerAtomicConverter<char, short>();
    registerAtomicConverter<char, unsigned short>();
    registerAtomicConverter<char, int32_t>();
    registerAtomicConverter<char, uint32_t>();
    registerAtomicConverter<char, int64_t>();
    registerAtomicConverter<char, uint64_t>();
    registerAtomicConverter<char, float>();
    registerAtomicConverter<char, double>();
    // byte
    registerAtomicConverter<byte, short>();
    registerAtomicConverter<byte, unsigned short>();
    registerAtomicConverter<byte, int32_t>();
    registerAtomicConverter<byte, uint32_t>();
    registerAtomicConverter<byte, int64_t>();
    registerAtomicConverter<byte, uint64_t>();
    registerAtomicConverter<byte, float>();
    registerAtomicConverter<byte, double>();
    // short
    registerAtomicConverter<short, unsigned short>();
    registerAtomicConverter<short, int32_t>();
    registerAtomicConverter<short, uint32_t>();
    registerAtomicConverter<short, int64_t>();
    registerAtomicConverter<short, uint64_t>();
    registerAtomicConverter<short, float>();
    registerAtomicConverter<short, double>();
    // word
    registerAtomicConverter<unsigned short, int32_t>();
    registerAtomicConverter<unsigned short, uint32_t>();
    registerAtomicConverter<unsigned short, int64_t>();
    registerAtomicConverter<unsigned short, uint64_t>();
    registerAtomicConverter<unsigned short, float>();
    registerAtomicConverter<unsigned short, double>();
    // int
    registerAtomicConverter<int32_t, uint32_t>();
    registerAtomicConverter<int32_t, int64_t>();
    registerAtomicConverter<int32_t, uint64_t>();
    registerAtomicConverter<int32_t, float>();
    registerAtomicConverter<int32_t, double>();
    // uint
    registerAtomicConverter<uint32_t, int64_t>();
    registerAtomicConverter<uint32_t, uint64_t>();
    registerAtomicConverter<uint32_t, float>();
    registerAtomicConverter<uint32_t, double>();
    // int64
    registerAtomicConverter<int64_t, uint64_t>();
    registerAtomicConverter<int64_t, float>();
    registerAtomicConverter<int64_t, double>();
    // uint64
    registerAtomicConverter<uint64_t, float>();
    registerAtomicConverter<uint64_t, double>();
    // float and double
    registerAtomicConverter<float, double>();
    // string
    internal_registerConverter<bool, std::string>(boolToString);
    internal_registerConverter<std::string, bool>(stringToBool);
    internal_registerConverter<std::string_view, bool>(literalToBool);
    registerStringConverter<byte>();
    registerStringConverter<short>();
    registerStringConverter<unsigned short>();
    registerStringConverter<int32_t>();
    registerStringConverter<uint32_t>();
    registerStringConverter<int64_t>();
    registerStringConverter<uint64_t>();
    registerStringConverter<float>();
    registerStringConverter<double>();
    // literal to string
    internal_registerConverter<std::string_view, std::string>(literalToString);
}

} // namespace mox
