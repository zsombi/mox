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

#include <private/process_p.hpp>
#include <mox/core/process/thread_loop.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/process/application.hpp>

#include <future>

namespace mox
{

void ThreadLoopPrivate::threadMain(ThreadPromise threadDataReady)
{    
    CTRACE(threads, "Preparing thread");
    P_PTR(ThreadLoop);

    p->setUp();
    auto loop = as_shared<RunLoop>(runLoop);

    // Notify thread launcher about the thread data readyness.
    CTRACE(threads, "Thread data ready");
    threadDataReady.set_value();

    CTRACE(threads, "Notify thread starter");
    p->started(p);

    auto status = statusProperty.get();
    if (status == ThreadInterface::Status::StartingUp)
    {
        CTRACE(threads, "Thread running" << intptr_t(thread.native_handle()));
        statusProperty = ThreadInterface::Status::Running;
        loop->execute();
        statusProperty = ThreadInterface::Status::Stopped;
        CTRACE(threads, "Thread stopped");
    }
    else
    {
        loop->execute(ProcessFlags::SingleLoop);
        CTRACE(threads, "Thread not started due to status:" << int(status));
    }

    p->stopped(p);
    p->tearDown();
}

/******************************************************************************
 *
 */
ThreadLoop::ThreadLoop()
    : ThreadInterface(pimpl::make_d_ptr<ThreadLoopPrivate>(this))
{
}

RunLoopBasePtr ThreadLoop::createRunLoopOverride()
{
    return RunLoop::create(false);
}

void ThreadLoop::startOverride()
{
    ThreadPromise tdReadyNotify;
    ThreadFuture tdReady = tdReadyNotify.get_future();

    D();
    d->thread = std::thread(&ThreadLoopPrivate::threadMain, d, std::move(tdReadyNotify));
    tdReady.wait();
}

void ThreadLoop::joinOverride()
{
    D();
    throwIf<ExceptionType::DetachedThread>(!d->thread.joinable());
    d->thread.join();
}

ThreadLoopPtr ThreadLoop::create()
{
    return make_thread(new ThreadLoop);
}

/******************************************************************************
 *
 */
LogLine& operator<<(LogLine& logger, ThreadLoopPtr thread)
{
    if (!logger.isEnabled())
    {
        return logger;
    }

    std::ostringstream ss;
    auto d = ThreadLoopPrivate::get(*thread);

    ss << "Thread(" << d->thread.native_handle() << ")";
    logger << ss.str();
    return logger;
}

}
