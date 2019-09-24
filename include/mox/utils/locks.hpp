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

#if !defined(MOX_SINGLE_THREADED)
#include <mutex>
#endif

namespace mox
{

#if defined(MOX_SINGLE_THREADED)

class ObjectLock
{
public:
    virtual ~ObjectLock() = default;

    virtual void lock()
    {
    }

    virtual void unlock()
    {
    }
};

template<typename LockType>
class ScopeLock
{
public:
    explicit ScopeLock(LockType& lock)
        : m_lock(lock)
    {
        m_lock.lock();
    }
    ~ScopeLock()
    {
        m_lock.unlock();
    }

    ScopeLock(const ScopeLock&) = delete;
    ScopeLock& operator=(const ScopeLock&) = delete;

private:
    LockType&  m_lock;
};

#else

using ObjectLock = std::mutex;
typedef std::lock_guard<ObjectLock> ScopeLock;

#endif // MOX_SINGLE_THREADED

template<typename LockType>
class ScopeRelock
{
public:
    explicit ScopeRelock(LockType& lock)
        : m_lock(lock)
    {
        m_lock.lock();
    }
    ~ScopeRelock()
    {
        m_lock.unlock();
    }

    ScopeRelock(const ScopeRelock&) = delete;
    ScopeRelock& operator=(const ScopeRelock&) = delete;

private:
    LockType&  m_lock;
};

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
