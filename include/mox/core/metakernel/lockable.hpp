// Copyright (C) 2020 bitWelder

#ifndef LOCKABLE_HPP
#define LOCKABLE_HPP

#include <mox/config/deftypes.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/utils/locks.hpp>

namespace mox { namespace metakernel {

class MOX_API Lockable : public mox::AtomicRefCounted<int32_t>
{
    using Base = mox::AtomicRefCounted<int32_t>;
    mutable std::mutex m_mutex;
#ifdef DEBUG
    mutable atomic_int32_t m_lockCount = 0;
    mutable std::atomic<std::thread::id> m_owner = std::thread::id();
#endif

public:
    explicit Lockable();
    virtual ~Lockable();

    void lock();
    void unlock();
    bool try_lock();
};

}} // mox::metakernel

#endif // LOCKABLE_HPP
