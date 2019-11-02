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

using Notifier = std::promise<void>;
using Waiter = std::future<void>;

ThreadLoop::ThreadLoop()
{
    addEventHandler(EventType::Quit, std::bind(&ThreadLoop::quitHandler, this, std::placeholders::_1));
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

void ThreadLoop::quitHandler(Event& event)
{
    QuitEvent& quit = static_cast<QuitEvent&>(event);
    m_threadData->m_exitCode.store(quit.getExitCode());
    m_threadData->m_eventDispatcher->stop();
}

void ThreadLoop::moveToThread()
{
    auto thisTD = ThreadData::thisThreadData();
    auto visitor = [&thisTD](Object& object)
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

bool ThreadLoop::isRunning() const
{
    lock_guard lock(const_cast<ThreadLoop&>(*this));
    return (m_status.load() == Status::Running);
}

void ThreadLoop::threadMain(std::promise<void> notifier)
{
    TRACE("Preparing thread")
    lock();
    ThreadDataSharedPtr threadData = ThreadData::create();
    threadData->m_thread = std::static_pointer_cast<ThreadLoop>(shared_from_this());
    unlock();
    moveToThread();

    started(this);

    // Notify caller thread that this thread is ready to receive messages.
    TRACE("Notify thread starter")
    notifier.set_value();
    TRACE("Thread running")
    run();
    TRACE("Thread stopped")

    TRACE("Thread really stopped")
    // The thread data is no longer valid, therefore reset it, and remove from all the objects owned by this thread loop.
    auto cleaner = [](Object& object)
    {
        lock_guard lock(object);
        object.m_threadData.reset();
        return Object::VisitResult::Continue;
    };
    traverseChildren(cleaner, TraverseOrder::InversePostOrder);
    stopped(this);
    threadData->m_thread.reset();
    m_threadData.reset();
}

void ThreadLoop::start(bool detached)
{
    Notifier notifier;
    Waiter notifierLock = notifier.get_future();
    m_thread = std::thread(&ThreadLoop::threadMain, this, std::move(notifier));

    // Wait till future gets signalled.
    notifierLock.wait();
    if (detached)
    {
        m_thread.detach();
    }
}

void ThreadLoop::exit(int exitCode)
{
    ThreadDataSharedPtr td = threadData();
    if (!td || (m_status.load() == Status::Stopped))
    {
        return;
    }
    postEvent<QuitEvent>(shared_from_this(), exitCode);
}

bool ThreadLoop::join(bool force)
{
    if (ThreadData::thisThreadData() == m_threadData)
    {
        std::cerr << "Cannot join a thead from within the running thread!" << std::endl;
        return false;
    }
    if (!m_thread.joinable() && !force)
    {
        std::cerr << "Detached thread must join forced." << std::endl;
        return false;
    }

    if (m_thread.joinable())
    {
        TRACE("Joining thread")
        m_thread.join();
        TRACE("Thread joined with success")
    }
    else if (force)
    {
        TRACE("Waiting detached thread to stop")
        std::promise<void> notifyer;
        auto notifyerWait = notifyer.get_future();
        auto watchThreadDown =[&notifyer]()
        {
            notifyer.set_value();
        };
        stopped.connect(watchThreadDown);
        notifyerWait.wait();
        TRACE("Detached thread stop noticed")
    }

    return true;
}

bool ThreadLoop::exitAndJoin(int exitCode)
{
    exit(exitCode);
    return join(true);
}

int ThreadLoop::run()
{
    EventLoop loop;
    m_status.store(Status::Running);
    loop.processEvents();
    m_status.store(Status::Stopped);
    return threadData()->exitCode();
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
