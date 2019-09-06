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

template <typename From, typename To>
void registerAtomicConverter()
{
    mox::registerConverter<From, To>(atomicConverter<From, To>);
    mox::registerConverter<To, From>(atomicConverter<To, From>);
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
        std::istringstream istream(value.data());
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
        if (istream.fail())
        {
            throw bad_conversion(metaType<String>(), metaType<To>());
        }
    }
    return result;
}

std::ostringstream& operator <<(std::ostringstream& os, byte value)
{
    os << static_cast<unsigned char>(value);
    return os;
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
    mox::registerConverter<std::string, T>(stringToNumber<std::string, T>);
    mox::registerConverter<T, std::string>(numberToString<T, std::string>);
    mox::registerConverter<std::string_view, T>(stringToNumber<std::string_view, T>);
}

std::string literalToString(std::string_view value)
{
    return value.data();
}

bad_conversion::bad_conversion(Metatype from, Metatype to)
{
    std::ostringstream ss;
    ss << "No converter found to convert from "
       << MetatypeDescriptor::get(from).name()
       << " to "
       << MetatypeDescriptor::get(to).name();
    m_message = ss.str();
}
const char* bad_conversion::what() const _NOEXCEPT
{
    return m_message.data();
}

// Registrar function
void registerConverters()
{
    // bool
    registerAtomicConverter<bool, char>();
    registerAtomicConverter<bool, byte>();
    registerAtomicConverter<bool, short>();
    registerAtomicConverter<bool, unsigned short>();
    registerAtomicConverter<bool, Int32>();
    registerAtomicConverter<bool, UInt32>();
    registerAtomicConverter<bool, long>();
    registerAtomicConverter<bool, unsigned long>();
    registerAtomicConverter<bool, Int64>();
    registerAtomicConverter<bool, UInt64>();
    registerAtomicConverter<bool, float>();
    registerAtomicConverter<bool, double>();
    // char
    registerAtomicConverter<char, byte>();
    registerAtomicConverter<char, short>();
    registerAtomicConverter<char, unsigned short>();
    registerAtomicConverter<char, Int32>();
    registerAtomicConverter<char, UInt32>();
    registerAtomicConverter<char, long>();
    registerAtomicConverter<char, unsigned long>();
    registerAtomicConverter<char, Int64>();
    registerAtomicConverter<char, UInt64>();
    registerAtomicConverter<char, float>();
    registerAtomicConverter<char, double>();
    // byte
    registerAtomicConverter<byte, short>();
    registerAtomicConverter<byte, unsigned short>();
    registerAtomicConverter<byte, Int32>();
    registerAtomicConverter<byte, UInt32>();
    registerAtomicConverter<byte, long>();
    registerAtomicConverter<byte, unsigned long>();
    registerAtomicConverter<byte, Int64>();
    registerAtomicConverter<byte, UInt64>();
    registerAtomicConverter<byte, float>();
    registerAtomicConverter<byte, double>();
    // short
    registerAtomicConverter<short, unsigned short>();
    registerAtomicConverter<short, Int32>();
    registerAtomicConverter<short, UInt32>();
    registerAtomicConverter<short, long>();
    registerAtomicConverter<short, unsigned long>();
    registerAtomicConverter<short, Int64>();
    registerAtomicConverter<short, UInt64>();
    registerAtomicConverter<short, float>();
    registerAtomicConverter<short, double>();
    // word
    registerAtomicConverter<unsigned short, Int32>();
    registerAtomicConverter<unsigned short, UInt32>();
    registerAtomicConverter<unsigned short, long>();
    registerAtomicConverter<unsigned short, unsigned long>();
    registerAtomicConverter<unsigned short, Int64>();
    registerAtomicConverter<unsigned short, UInt64>();
    registerAtomicConverter<unsigned short, float>();
    registerAtomicConverter<unsigned short, double>();
    // int
    registerAtomicConverter<Int32, UInt32>();
    registerAtomicConverter<Int32, long>();
    registerAtomicConverter<Int32, unsigned long>();
    registerAtomicConverter<Int32, Int64>();
    registerAtomicConverter<Int32, UInt64>();
    registerAtomicConverter<Int32, float>();
    registerAtomicConverter<Int32, double>();
    // uint
    registerAtomicConverter<UInt32, long>();
    registerAtomicConverter<UInt32, unsigned long>();
    registerAtomicConverter<UInt32, Int64>();
    registerAtomicConverter<UInt32, UInt64>();
    registerAtomicConverter<UInt32, float>();
    registerAtomicConverter<UInt32, double>();
    // long
    registerAtomicConverter<long, unsigned long>();
    registerAtomicConverter<long, Int64>();
    registerAtomicConverter<long, UInt64>();
    registerAtomicConverter<long, float>();
    registerAtomicConverter<long, double>();
    // ulong
    registerAtomicConverter<unsigned long, Int64>();
    registerAtomicConverter<unsigned long, UInt64>();
    registerAtomicConverter<unsigned long, float>();
    registerAtomicConverter<unsigned long, double>();
    // int64
    registerAtomicConverter<Int64, UInt64>();
    registerAtomicConverter<Int64, float>();
    registerAtomicConverter<Int64, double>();
    // uint64
    registerAtomicConverter<UInt64, float>();
    registerAtomicConverter<UInt64, double>();
    // float and double
    registerAtomicConverter<float, double>();
    // string
    registerConverter<bool, std::string>(boolToString);
    registerConverter<std::string, bool>(stringToBool);
    registerConverter<std::string_view, bool>(literalToBool);
    registerStringConverter<byte>();
    registerStringConverter<short>();
    registerStringConverter<unsigned short>();
    registerStringConverter<Int32>();
    registerStringConverter<UInt32>();
    registerStringConverter<long>();
    registerStringConverter<unsigned long>();
    registerStringConverter<Int64>();
    registerStringConverter<UInt64>();
    registerStringConverter<float>();
    registerStringConverter<double>();
    // literal to string
    registerConverter<std::string_view, std::string>(literalToString);
}

} // namespace mox
