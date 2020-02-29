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

}

#endif // DEFTYPES_HPP
