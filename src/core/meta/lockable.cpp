// Copyright (C) 2020 bitWelder

#include <mox/core/meta/lockable.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

Lockable::Lockable()
    : Base(0)
{
}

#ifdef DEBUG

Lockable::~Lockable()
{
    FATAL(m_lockCount.load() == 0, "Destroying unlocked object! LockCount is " << m_lockCount.load());
    m_lockCount.store(-1);
}

void Lockable::lock()
{
    FATAL(m_lockCount >= 0, "Invalid MetaBase");

    if (!try_lock())
    {
        // Is this the same owner?
        if (m_owner == std::this_thread::get_id())
        {
            FATAL(false, "Deadlocked MetaBase! LockCount is " << m_lockCount);
        }
        m_mutex.lock();
        m_owner = std::this_thread::get_id();
        m_lockCount++;
    }
}

void Lockable::unlock()
{
    FATAL(m_lockCount > 0, "Cannot unlock MetaBase if not locked! LockCount is " << m_lockCount);
    m_mutex.unlock();
    m_lockCount--;
    m_owner = std::thread::id();
}

bool Lockable::try_lock()
{
    auto result = m_mutex.try_lock();
    if (result)
    {
        m_lockCount++;
        m_owner = std::this_thread::get_id();
    }
    return result;
}

#else

Lockable::~Lockable()
{
}

void Lockable::lock()
{
    m_mutex.lock();
}

void Lockable::unlock()
{
    m_mutex.unlock();
}

bool Lockable::try_lock()
{
    return m_mutex.try_lock();
}

#endif
} // mox
