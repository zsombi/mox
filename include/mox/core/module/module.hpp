/*
 * Copyright (C) 2017-2018 bitWelder
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

#ifndef MODULE_HPP
#define MODULE_HPP

#include <mox/config/platform_config.hpp>

namespace mox
{

/// Generic module API.
class MOX_API Module
{
public:
    virtual ~Module() = default;
    /// Registers your module.
    virtual void registerModule() = 0;

protected:
    explicit Module() = default;
};

}

#endif // MODULE_HPP
