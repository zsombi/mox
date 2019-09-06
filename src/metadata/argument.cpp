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

#include "metadata_p.h"

#include <mox/metadata/argument.hpp>

namespace mox
{

Argument::Argument(const Argument& other)
    : m_data(other.m_data)
{
}

bool Argument::isValid() const
{
    return m_data && m_data->m_value.has_value();
}

void Argument::reset()
{
    m_data.reset();
}

Metatype Argument::metaType() const
{
    ASSERT(m_data, "Argument is not initialized.");
    return m_data->m_typeDescriptor.type;
}

const ArgumentDescriptor& Argument::descriptor() const
{
    ASSERT(m_data, "Argument is not initialized.");
    return m_data->m_typeDescriptor;
}

} // namespace mox
