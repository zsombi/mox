// Copyright (C) 2020 bitWelder

#include <mox/core/meta/lockable.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

#ifdef DEBUG

Lockable::Lockable()
    : Base(1)
{
}

Lockable::~Lockable()
{
    FATAL(m_value.load() == 1, "Destroying a shared locked object! Share count is" << m_value.load());
    m_value.store(-999);
}

void Lockable::lock()
{
    FATAL(m_value >= 1, "Attempt locking an invalid Lockable");

    if (!try_lock())
    {
        m_mutex.lock();
    }
}

void Lockable::unlock()
{
    FATAL(m_value >= 1, "Attempt unlocking an invalid Lockable");
    m_mutex.unlock();
}

bool Lockable::try_lock()
{
    FATAL(m_value >= 1, "Attempt locking an invalid Lockable");
    return m_mutex.try_lock();
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
