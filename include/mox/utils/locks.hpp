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

#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <mutex>

namespace mox
{

typedef std::lock_guard<std::mutex> ScopeLock;

template <bool Value>
struct FlagScope
{
    explicit FlagScope(bool& flag)
        : m_flag(flag)
    {
        m_flag = Value;
    }
    ~FlagScope()
    {
        m_flag = !Value;
    }

private:
    bool& m_flag;
};

}

#endif // LOCKS_HPP
