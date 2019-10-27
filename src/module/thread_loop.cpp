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

#include <mox/module/thread_loop.hpp>
#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/event_loop.hpp>

#include <future>

namespace mox
{

ThreadLoop::ThreadLoop()
{
}
ThreadLoop::~ThreadLoop()
{
    TRACE("ThreadLoop object: destroying")
    exitAndJoin();
    TRACE("ThreadLoop object: destroyed")
}

void ThreadLoop::registerModule()
{
}

Object::VisitResult ThreadLoop::moveToThread(ThreadDataSharedPtr threadData)
{
    UNUSED(threadData);
    // Skip the entire thread content. No objects parented to this thread object shall be moved
    // to other threads.
    return VisitResult::ContinueSibling;
}

void ThreadLoop::moveToThread()
{
    ThreadDataSharedPtr thisTD = ThreadData::thisThreadData();
    auto visitor = [thisTD](Object& object)
    {
        lock_guard lock(object);
        object.m_threadData = thisTD;
        return Object::VisitResult::Continue;
    };
    traverse(visitor, TraverseOrder::PreOrder);
}

ThreadLoop::Status ThreadLoop::getStatus() const
{
    return m_status.load();
}

void ThreadLoop::threadMain(std::promise<void> notifier)
{
    TRACE("Preparing thread")
    ThreadDataSharedPtr threadData = ThreadData::create();
    threadData->m_thread = std::static_pointer_cast<ThreadLoop>(shared_from_this());
    moveToThread();

    started(this);

    // Notify caller thread that this thread is ready to receive messages.
    TRACE("Notify thread starter")
    notifier.set_value();
    TRACE("Thread running")
    run();
    TRACE("Thread stopped")

    stopped(this);
    TRACE("Thread really died. Destroying ThreadData")
}

ThreadLoop& ThreadLoop::start()
{
    std::promise<void> notifier;
    std::future<void> notifierLock = notifier.get_future();
    m_thread = std::thread(&ThreadLoop::threadMain, this, std::move(notifier));

    // Wait till future gets signalled.
    notifierLock.wait();
    return *this;
}

void ThreadLoop::exit(int exitCode)
{
    ThreadDataSharedPtr td = threadData();
    if (!td)
    {
        return;
    }
    EventLoopPtr loop = td->eventLoop();
    if (loop)
    {
        loop->exit(exitCode);
    }
}

void ThreadLoop::exitAndJoin(int exitCode)
{
    if (m_status.load() != Status::Running)
    {
        return;
    }

    if (ThreadData::thisThreadData() == m_threadData.lock())
    {
        // Cannot join.
        std::cerr << "Cannot join a thead from inside the running thread!" << std::endl;
        return;
    }

    if (m_thread.joinable())
    {
        exit(exitCode);
        m_thread.join();
    }
    else
    {
        std::promise<void> notifyer;
        auto notifyerWait = notifyer.get_future();
        auto watchThreadDown =[&notifyer]()
        {
            notifyer.set_value();
        };
        stopped.connect(watchThreadDown);
        exit(exitCode);
        notifyerWait.wait();
    }
}

void ThreadLoop::run()
{
    EventLoop loop;
    m_status.store(Status::Running);
    loop.processEvents();
    m_status.store(Status::Stopped);
}

ThreadLoopSharedPtr ThreadLoop::create(Object* parent)
{
    return createObject(new ThreadLoop, parent);
}

ThreadLoopSharedPtr ThreadLoop::thisThread()
{
    return ThreadData::thisThreadData()->thread();
}

}
