/*
 * Copyright (C) 2017-2020 bitWelder
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

#ifndef PROCESS_P_HPP
#define PROCESS_P_HPP

#include <mox/core/process/thread_interface.hpp>
#include <mox/core/process/thread_loop.hpp>

namespace mox
{

class ThreadInterfacePrivate
{
public:
    DECLARE_PUBLIC_PTR(ThreadInterface);

    using AttachedThreadsColletcion = std::vector<ThreadInterfacePtr>;

    EventQueue threadQueue;
    AttachedThreadsColletcion childThreads;
    AtomicPropertyData<ThreadInterface::Status> statusProperty;
    AtomicPropertyData<int> exitCodeProperty;
    RunLoopBasePtr runLoop;

    explicit ThreadInterfacePrivate(ThreadInterface* pp);
    ~ThreadInterfacePrivate() = default;
};

class ThreadLoopPrivate : public ThreadInterfacePrivate
{
public:
    DECLARE_PUBLIC(ThreadLoop, ThreadLoopPrivate);

    explicit ThreadLoopPrivate(ThreadLoop* p)
        : ThreadInterfacePrivate(p)
    {
    }
    void threadMain(ThreadPromise threadDataReady);

    mutable std::thread thread;
};

namespace td
{

void attachToThread(ThreadData& td);
void detachFromThread();

}

}

#endif // PROCESS_P_HPP
