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

namespace mox
{

/// The Argument class holds a value passed as argument to metacalls.
struct MOX_API Argument
{
    typedef std::function<void*()> GetterFunction;

    /// Constructor.
    explicit Argument() = default;

    /// Templated constructor, initializes the argument with a given value.
    template <typename T>
    explicit Argument(const T value)
        : m_data(std::make_shared<Data>(value))
    {
    }

    /// Copy constructor.
    Argument(const Argument& other);

    /// Chacks if the argument is convertible into type T.
    /// \return \e true if the argument is convertible into type T, \e false otherwise.
    template <typename T>
    bool canConvert()
    {
        return isValid() && registrar::findConverter(metaType(), mox::metaType<T>()) != nullptr;
    }

    /// Cast operator, returns the value stored by the argument, or the value converted
    /// in the desired type.
    /// \throws bad_conversion if the argument is not convertible to the requested type.
    template <typename T>
    operator T()
    {
        ASSERT(m_data, "Argument is not initialized.");
        return m_data->get<T>();
    }

    /// Const cast operator, returns the value stored by the argument, or the value converted
    /// in the desired type.
    /// \throws bad_conversion if the argument is not convertible to the requested type.
    template <typename T>
    operator T() const
    {
        ASSERT(m_data, "Argument is not initialized.");
        return m_data->get<T>();
    }

    /// Assignment operator.
    template <typename T>
    Argument& operator=(const T value)
    {
        static_assert (!is_cstring<T>::value, "Argument cannot hold a cstring.");
        reset();
        m_data = std::make_shared<Data>(value);
        return *this;
    }

    /// Returns \e true if the argument holds a valid value.
    bool isValid() const;

    /// Reset the argument.
    void reset();

    /// Returns the metatype of the value held by this argument.
    Metatype metaType() const;

    /// Returns the argumemnt descriptor.
    const ArgumentDescriptor& descriptor() const;

private:
    struct Data
    {
        ArgumentBase m_value;
        mutable GetterFunction m_getter;
        ArgumentDescriptor m_typeDescriptor;

        template <typename T>
        explicit Data(T value)
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
        T get()
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
            ArgumentBase tmp = converter->convert(*converter, sourceValue);
            value = std::any_cast<T>(&tmp);
            if (!value)
            {
                throw bad_conversion(sourceType, destinationType);
            }

            return *value;
        }
    };

    std::shared_ptr<Data> m_data;
};

template <typename T>
bool operator==(T const& value, const Argument& arg)
{
    const T v = arg;
    return value == v;
}

template <typename T>
bool operator==(const Argument& arg, T const& value)
{
    const T v = arg;
    return value == v;
}

template <typename T>
bool operator!=(T const& value, const Argument& arg)
{
    const T v = arg;
    return value != v;
}

template <typename T>
bool operator!=(const Argument& arg, T const& value)
{
    const T v = arg;
    return value != v;
}

} // namespace mox

#endif // ANY_HPP
