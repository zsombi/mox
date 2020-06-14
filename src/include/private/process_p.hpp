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

class StatusDP : public ThreadInterface::ThreadStatus::Data
{
    ThreadInterface::Status status = ThreadInterface::Status::InactiveOrJoined;

public:

    ThreadInterface::Status get() const override
    {
        return status;
    }

    operator ThreadInterface::Status() const
    {
        return status;
    }
    void operator=(ThreadInterface::Status value)
    {
        status = value;
        update();
    }
};

class ExitDP : public ThreadInterface::ExitCode::Data
{
    int exitCode = 0;
public:
    int get() const override
    {
        return exitCode;
    }
    operator int() const
    {
        return exitCode;
    }

    void operator=(int code)
    {
        exitCode = code;
        update();
    }
};

class ThreadInterfacePrivate
{
public:
    DECLARE_PUBLIC_PTR(ThreadInterface);

    using AttachedThreadsColletcion = std::vector<ThreadInterfacePtr>;

    EventQueue threadQueue;
    AttachedThreadsColletcion childThreads;
    StatusDP statusProperty;
    ExitDP exitCodeProperty;
    RunLoopBasePtr runLoop;

    explicit ThreadInterfacePrivate(ThreadInterface* pp);
    virtual ~ThreadInterfacePrivate() = default;

    void attachToParentThread();
    void detachFromParentThread();
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
