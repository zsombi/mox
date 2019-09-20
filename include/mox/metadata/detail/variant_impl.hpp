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
    return isValid() && registrar::findConverter(metaType(), mox::metaType<T>()) != nullptr;
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
    if constexpr (std::is_pointer_v<T>)
    {
        m_getter = [this]() -> void*
        {
            T ret = std::any_cast<T>(m_value);
            return reinterpret_cast<void*>(ret);
        };
    }
    else
    {
        m_getter = [this]() -> void*
        {
            T* ret = std::any_cast<T>(&m_value);
            return reinterpret_cast<void*>(ret);
        };
    }
}

template <typename T>
T Variant::Data::get()
{
    T* value = std::any_cast<T>(&m_value);
    if (value)
    {
        return *value;
    }

    Metatype sourceType = m_typeDescriptor.type;
    Metatype destinationType = mox::metaType<T>();
    MetatypeConverter* converter = registrar::findConverter(sourceType, destinationType);
    if (!converter)
    {
        throw bad_conversion(sourceType, destinationType);
    }

    void* sourceValue = m_getter();
    MetaValue tmp = converter->convert(*converter, sourceValue);
    value = std::any_cast<T>(&tmp);
    if (!value)
    {
        throw bad_conversion(sourceType, destinationType);
    }

    return *value;
}

} // namespace mox

#endif // ARGUMENT_IMPL_HPP
