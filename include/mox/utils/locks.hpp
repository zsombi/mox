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

#include <mox/config/platform_config.hpp>
#include <mox/utils/ref_counted.hpp>

#include <functional>
#include <thread>

namespace mox
{

using std::atomic;
using atomic_bool = std::atomic_bool;
using atomic_int32_t = std::atomic_int32_t;
using std::lock_guard;

/// Base class sharing the locking with an MetaBase.
template <typename RefLockable>
class MOX_API SharedLock : public RefCounter<RefLockable&>
{
    using BaseClass = RefCounter<RefLockable&>;
    DISABLE_MOVE(SharedLock)
    DISABLE_COPY(SharedLock)

public:
    /// Construct the shared lock with the given \a sharedLock. Does not retain the lock, but
    /// increases the shared count of the object lock.
    explicit SharedLock(RefLockable& sharedLock)
      : BaseClass(sharedLock)
    {
    }
    /// Destructs the shared lock and releases the object lock shared count.
    virtual ~SharedLock() = default;

    /// Locks the shared object.
    void lock()
    {
        this->m_refCounted.lock();
    }
    /// Unlocks the shared object.
    void unlock()
    {
        this->m_refCounted.unlock();
    }
    /// Tries to lock the shared object.
    /// \return If the object locking succeeds, returns \e true, otherwise \e false.
    bool try_lock()
    {
        return this->m_refCounted.try_lock();
    }
};

/// The template works the opposite to the std::lock_guard class: unlocks the lock on construction
/// and re-locks on destruction.
template<typename LockType>
class ScopeRelock
{
public:
    /// Constructor, unlocks the \a lock.
    explicit ScopeRelock(LockType& lock)
        : m_lock(lock)
    {
        m_lock.unlock();
    }

    /// Destructor, re-locks the lock.
    ~ScopeRelock()
    {
        m_lock.lock();
    }

private:
    DISABLE_COPY(ScopeRelock)
    DISABLE_MOVE(ScopeRelock)
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

template <typename T, typename V>
struct ScopeValue
{
    explicit ScopeValue(T& arg, V value)
        : m_arg(arg)
        , m_prevValue(V(m_arg))
    {
        m_arg = value;
    }
    ~ScopeValue()
    {
        m_arg = m_prevValue;
    }

private:
    T& m_arg;
    V m_prevValue;
};

}

#endif // LOCKS_HPP
