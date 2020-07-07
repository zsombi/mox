// Copyright (C) 2020 bitWelder

#include <mox/core/meta/lockable.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

#ifdef DEBUG

Lockable::Lockable()
    : Base(0)
{
}

Lockable::~Lockable()
{
    FATAL(m_value.load() == 0, "Destroying locked object! LockCount is" << m_value.load());
    m_value.store(-999);
}

void Lockable::lock()
{
    FATAL(m_value >= 0, "Invalid Lockable");

    if (!try_lock())
    {
        // Is this the same owner?
        FATAL(m_owner != std::this_thread::get_id(), "Deadlocked Lockable! LockCount is " << m_value);
        m_mutex.lock();
        m_owner = std::this_thread::get_id();
        retain();
    }
}

void Lockable::unlock()
{
    FATAL(m_value > 0, "Cannot unlock Lockable if not locked! LockCount is " << m_value);
    FATAL(m_owner == std::this_thread::get_id(), "About to unlock Lockable from a different thread!");

    release();
    m_owner = std::thread::id();
    m_mutex.unlock();
}

bool Lockable::try_lock()
{
    FATAL(m_value >= 0, "Corrupted Lockable! LockCount is " << m_value);
    auto result = m_mutex.try_lock();
    if (result)
    {
        retain();
        m_owner = std::this_thread::get_id();
    }
    return result;
}

#else

Lockable::Lockable()
{
}

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
