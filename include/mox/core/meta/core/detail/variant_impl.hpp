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
#ifndef ARGUMENT_IMPL_HPP
#define ARGUMENT_IMPL_HPP

#include <mox/config/error.hpp>

namespace mox
{

template <typename T>
Variant::Variant(const T value)
    : m_data(std::make_shared<Data>(value))
{
}

template <typename T>
bool Variant::canConvert()
{
    return isValid() && MetatypeDescriptor::get(metaType()).findConverterTo(mox::metaType<T>()) != nullptr;
}

template <typename T>
Variant::operator T()
{
    FATAL(m_data, "Variant is not initialized.");
    return m_data->get<T>();
}

template <typename T>
Variant::operator T() const
{
    FATAL(m_data, "Variant is not initialized.");
    return m_data->get<T>();
}

template <typename T>
Variant& Variant::operator=(const T value)
{
    static_assert (!is_cstring<T>::value, "Variant cannot hold a cstring.");
    reset();
    m_data = std::make_shared<Data>(value);
    return *this;
}


template <typename T>
Variant::Data::Data(T value)
    : m_value(value)
    , m_typeDescriptor(value)
{
    m_getter = [value]() -> void*
    {
        if constexpr (std::is_pointer_v<T>)
        {
            return reinterpret_cast<void*>(const_cast<T>(value));
        }
        else
        {
            return reinterpret_cast<void*>(const_cast<T*>(&value));
        };
    };

    m_isEqual = [this](const Data& other)
    {
        T otherValue = std::any_cast<T>(other.m_value);
        T thisValue = std::any_cast<T>(m_value);
        return otherValue == thisValue;
    };
}

template <typename T>
T Variant::Data::get()
{
    T* value = std::any_cast<T>(&m_value);
    if (value)
    {
        return *value;
    }

    const auto& sourceType = MetatypeDescriptor::get(m_typeDescriptor.getType());
    auto destinationType = mox::metaType<T>();
    auto converter = sourceType.findConverterTo(destinationType);
    throwIf<ExceptionType::BadTypeConversion>(!converter);

    void* sourceValue = m_getter();
    auto tmp = converter->convert(sourceValue);
    value = std::any_cast<T>(&tmp);
    throwIf<ExceptionType::BadTypeConversion>(!value);

    return *value;
}

} // namespace mox

#endif // ARGUMENT_IMPL_HPP
