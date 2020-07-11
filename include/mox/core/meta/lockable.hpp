// Copyright (C) 2020 bitWelder

#ifndef LOCKABLE_HPP
#define LOCKABLE_HPP

#include <mox/config/deftypes.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/utils/locks.hpp>

namespace mox
{

#ifdef DEBUG
class MOX_API Lockable : public mox::AtomicRefCounted<int32_t>
#else
class MOX_API Lockable
#endif
{
    mutable std::mutex m_mutex;
#ifdef DEBUG
    using Base = mox::AtomicRefCounted<int32_t>;
#endif

public:
    explicit Lockable();
    virtual ~Lockable();

    void lock();
    void unlock();
    bool try_lock();
};

} // mox

#endif // LOCKABLE_HPP
