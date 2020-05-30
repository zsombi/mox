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
    FATAL(m_value.load() == 0, "Destroying unlocked object! LockCount is " << m_value.load());
    m_value.store(-1);
}

void Lockable::lock()
{
    FATAL(m_value >= 0, "Invalid MetaBase");

    if (!try_lock())
    {
        // Is this the same owner?
        FATAL(m_owner != std::this_thread::get_id(), "Deadlocked MetaBase! LockCount is " << m_value);
        m_mutex.lock();
        m_owner = std::this_thread::get_id();
        retain();
    }
}

void Lockable::unlock()
{
    FATAL(m_value > 0, "Cannot unlock MetaBase if not locked! LockCount is " << m_value);
    release();
    m_owner = std::thread::id();
    m_mutex.unlock();
}

bool Lockable::try_lock()
{
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
