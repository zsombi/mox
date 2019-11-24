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

#include <mox/utils/locks.hpp>

namespace mox
{

ObjectLock::ObjectLock()
{
    m_lockCount.store(0);
}

ObjectLock::~ObjectLock()
{
    FATAL(m_lockCount.load() == 0, "Destroying unlocked object! LockCount is " << m_lockCount.load())
}

void ObjectLock::lock()
{
//    m_mutex.lock();
    if (!try_lock())
    {
        FATAL(false, "Already locked ObjectLock! LockCount is " << m_lockCount)
    }
}

void ObjectLock::unlock()
{
    FATAL(m_lockCount > 0, "Cannot unlock ObjectLock if not locked! LockCount is " << m_lockCount)
    m_mutex.unlock();
    m_lockCount--;
}

bool ObjectLock::try_lock()
{
    auto result = m_mutex.try_lock();
    if (result)
    {
        m_lockCount++;
    }
    return result;
}

}
