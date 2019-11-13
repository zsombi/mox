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
    UNUSED(value);
    return static_cast<To>(value);
}

template <typename From, typename To, typename Function>
void internal_registerConverter(MetaData& metaData, Function converter)
{
    UNUSED(metaData);
    MetatypeDescriptor* fromType = metadata::findMetatypeDescriptor(metadata::remove_cv<From>());
    Metatype to = metadata::findMetatypeDescriptor(metadata::remove_cv<To>())->id();
    fromType->addConverter(std::make_unique<ConverterFunctor<From, To, Function>>(converter), to);
}

template <typename From, typename To>
void registerAtomicConverter(MetaData& metaData)
{
    internal_registerConverter<From, To>(metaData, atomicConverter<From, To>);
    internal_registerConverter<To, From>(metaData, atomicConverter<To, From>);
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
        if (istream.fail())
        {
            throw bad_conversion(metaType<String>(), metaType<To>());
        }
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
void registerStringConverter(MetaData& metaData)
{
    internal_registerConverter<std::string, T>(metaData, stringToNumber<std::string, T>);
    internal_registerConverter<T, std::string>(metaData, numberToString<T, std::string>);
    internal_registerConverter<std::string_view, T>(metaData, stringToNumber<std::string_view, T>);
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

const char* bad_conversion::what() const EXCEPTION_NOEXCEPT
{
    return m_message.data();
}

// Registrar function
void registerConverters(MetaData& metaData)
{
    // bool
    registerAtomicConverter<bool, char>(metaData);
    registerAtomicConverter<bool, byte>(metaData);
    registerAtomicConverter<bool, short>(metaData);
    registerAtomicConverter<bool, unsigned short>(metaData);
    registerAtomicConverter<bool, int32_t>(metaData);
    registerAtomicConverter<bool, uint32_t>(metaData);
    registerAtomicConverter<bool, int64_t>(metaData);
    registerAtomicConverter<bool, uint64_t>(metaData);
    registerAtomicConverter<bool, float>(metaData);
    registerAtomicConverter<bool, double>(metaData);
    // char
    registerAtomicConverter<char, byte>(metaData);
    registerAtomicConverter<char, short>(metaData);
    registerAtomicConverter<char, unsigned short>(metaData);
    registerAtomicConverter<char, int32_t>(metaData);
    registerAtomicConverter<char, uint32_t>(metaData);
    registerAtomicConverter<char, int64_t>(metaData);
    registerAtomicConverter<char, uint64_t>(metaData);
    registerAtomicConverter<char, float>(metaData);
    registerAtomicConverter<char, double>(metaData);
    // byte
    registerAtomicConverter<byte, short>(metaData);
    registerAtomicConverter<byte, unsigned short>(metaData);
    registerAtomicConverter<byte, int32_t>(metaData);
    registerAtomicConverter<byte, uint32_t>(metaData);
    registerAtomicConverter<byte, int64_t>(metaData);
    registerAtomicConverter<byte, uint64_t>(metaData);
    registerAtomicConverter<byte, float>(metaData);
    registerAtomicConverter<byte, double>(metaData);
    // short
    registerAtomicConverter<short, unsigned short>(metaData);
    registerAtomicConverter<short, int32_t>(metaData);
    registerAtomicConverter<short, uint32_t>(metaData);
    registerAtomicConverter<short, int64_t>(metaData);
    registerAtomicConverter<short, uint64_t>(metaData);
    registerAtomicConverter<short, float>(metaData);
    registerAtomicConverter<short, double>(metaData);
    // word
    registerAtomicConverter<unsigned short, int32_t>(metaData);
    registerAtomicConverter<unsigned short, uint32_t>(metaData);
    registerAtomicConverter<unsigned short, int64_t>(metaData);
    registerAtomicConverter<unsigned short, uint64_t>(metaData);
    registerAtomicConverter<unsigned short, float>(metaData);
    registerAtomicConverter<unsigned short, double>(metaData);
    // int
    registerAtomicConverter<int32_t, uint32_t>(metaData);
    registerAtomicConverter<int32_t, int64_t>(metaData);
    registerAtomicConverter<int32_t, uint64_t>(metaData);
    registerAtomicConverter<int32_t, float>(metaData);
    registerAtomicConverter<int32_t, double>(metaData);
    // uint
    registerAtomicConverter<uint32_t, int64_t>(metaData);
    registerAtomicConverter<uint32_t, uint64_t>(metaData);
    registerAtomicConverter<uint32_t, float>(metaData);
    registerAtomicConverter<uint32_t, double>(metaData);
    // int64
    registerAtomicConverter<int64_t, uint64_t>(metaData);
    registerAtomicConverter<int64_t, float>(metaData);
    registerAtomicConverter<int64_t, double>(metaData);
    // uint64
    registerAtomicConverter<uint64_t, float>(metaData);
    registerAtomicConverter<uint64_t, double>(metaData);
    // float and double
    registerAtomicConverter<float, double>(metaData);
    // string
    internal_registerConverter<bool, std::string>(metaData, boolToString);
    internal_registerConverter<std::string, bool>(metaData, stringToBool);
    internal_registerConverter<std::string_view, bool>(metaData, literalToBool);
    registerStringConverter<byte>(metaData);
    registerStringConverter<short>(metaData);
    registerStringConverter<unsigned short>(metaData);
    registerStringConverter<int32_t>(metaData);
    registerStringConverter<uint32_t>(metaData);
    registerStringConverter<int64_t>(metaData);
    registerStringConverter<uint64_t>(metaData);
    registerStringConverter<float>(metaData);
    registerStringConverter<double>(metaData);
    // literal to string
    internal_registerConverter<std::string_view, std::string>(metaData, literalToString);
}

} // namespace mox
