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
#include <atomic>
#endif

#include <mox/utils/globals.hpp>

#include <functional>

namespace mox
{

#if defined(MOX_SINGLE_THREADED)

class ObjectLock
{
public:
    ~ObjectLock() = default;

    void lock()
    {
        FATAL(!m_locked, "Object already locked!")
        m_locked = true;
    }

    void unlock()
    {
        FATAL(m_locked, "Object already unlocked!")
        m_locked = false;
    }

    bool try_lock()
    {
        if (m_locked)
        {
            return false;
        }
        lock();
        return m_locked;
    }
private:
    bool m_locked = false;
};

template<typename LockType>
class lock_guard
{
public:
    explicit lock_guard(LockType& lock)
        : m_lock(lock)
    {
        m_lock.lock();
    }
    ~lock_guard()
    {
        m_lock.unlock();
    }

private:
    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;

    LockType&  m_lock;
};

// Emulate atomic.
template <typename T>
class atomic
{
public:
    T load() const
    {
        return m_value;
    }
    T load() const volatile
    {
        return m_value;
    }

    void store(T value)
    {
        m_value = value;
    }
    void store(T value) volatile
    {
        m_value = value;
    }

    T operator=(T value)
    {
        store(value);
    }
    T operator=(T value) volatile
    {
        store(value);
    }

    operator T()
    {
        return load();
    }
    operator T() volatile
    {
        return load();
    }

    explicit atomic() = default;
    explicit atomic(T value)
        : m_value(value)
    {
    }

    DISABLE_COPY(atomic)
    DISABLE_MOVE(atomic)

private:
    T m_value;
};

#else

using std::atomic;
using atomic_bool = std::atomic_bool;
using atomic_int32_t = std::atomic_int32_t;
using std::lock_guard;
using ObjectLock = std::mutex;

#endif // MOX_SINGLE_THREADED

/// The template unlocks the lock passed as argument on construction, and relocks
/// it on destruction.
template<typename LockType>
class ScopeUnlock
{
public:
    explicit ScopeUnlock(LockType& lock)
        : m_lock(lock)
    {
        m_lock.unlock();
    }
    ~ScopeUnlock()
    {
        m_lock.lock();
    }

private:
    DISABLE_COPY(ScopeUnlock)
    LockType&  m_lock;
};

/// The OrderedLock class is a scope lock locking two individual LockType instances.
/// The lock objects are locked in ascending ordered based on their address, the smaller
/// address lock is locked before the other. Just like with lock_guard, the object locks
/// are guaranteed to unlock on destruction.
template <typename LockType>
class OrderedLock
{
public:
    explicit OrderedLock(LockType* l1, LockType* l2)
        : m_l1((l1 == l2) ?      l1 : (std::less<LockType*>()(l1, l2) ? l1 : l2))
        , m_l2((l1 == l2) ? nullptr : (std::less<LockType*>()(l1, l2) ? l2 : l1))
    {
        if (m_l1) m_l1->lock();
        if (m_l2) m_l2->lock();
    }
    ~OrderedLock()
    {
        if (m_l1) m_l1->unlock();
        if (m_l2) m_l2->unlock();
    }

    DISABLE_COPY(OrderedLock)

private:
    LockType* m_l1 = nullptr;
    LockType* m_l2 = nullptr;
};

/// The class locks two mutexes so that it avoids eventual deadlocks that occur
/// in threads when the mutexes are locked in different order. Assumes that the
/// first mutex is locked. If the lock fails on the second mutex, it unlocks
/// the first mutex and then relocks the mutexes in the proper order.
template <typename LockType>
class OrderedRelock
{
public:
    explicit OrderedRelock(LockType l1, LockType l2)
    {
        // l1 is already locked, l2 not... do we need to unlock and relock?
        if (l1 == l2 || !l2)
        {
            // Do nothing;
            m_lock = nullptr;
        }
        else if (l1 < l2)
        {
            l2->lock();
            m_lock = l2;
        }
        else if (!l2->try_lock())
        {
            l1->unlock();
            l2->lock();
            l1->lock();
            m_lock = l2;
        }
    }
    ~OrderedRelock()
    {
        if (m_lock)
        {
            m_lock->unlock();
        }
    }

    DISABLE_COPY(OrderedRelock)

private:
    LockType* m_lock = nullptr;
};

/// Flips the flag value for the time of the scope lifetime.
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

    DISABLE_COPY(FlagScope)

private:
    bool& m_flag;
};

template <typename Type>
struct ValueScope
{
    using CleanupFunc = std::function<void()>;

    explicit ValueScope(Type& value, CleanupFunc cleanup = nullptr)
        : m_value(value)
        , m_cleanup(cleanup)
    {
        ++m_value;
    }
    ~ValueScope()
    {
        --m_value;
        if (m_value <= 0 && m_cleanup)
        {
            m_cleanup();
        }
    }
private:
    Type& m_value;
    CleanupFunc m_cleanup;
};

}

#endif // LOCKS_HPP
