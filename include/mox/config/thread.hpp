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

#ifndef THREAD_HPP
#define THREAD_HPP

#ifdef MOX_SINGLE_THREADED
#error "Mox built with single thread mode cannot use threads"
#endif

#include <thread>
#include <mox/config/platform_config.hpp>

namespace mox
{

class thread_differs : public std::exception
{
public:
    const char* what() const EXCEPTION_NOEXCEPT override
    {
        return "Object cannot be parented to a different thread";
    }
};

class thread_data_set : public std::exception
{
public:
    const char* what() const EXCEPTION_NOEXCEPT override
    {
        return "ThreadLoop is already set on this thread";
    }
};

}

#endif // THREAD_HPP
