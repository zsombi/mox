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

#include <process_p.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

ThreadInterfacePrivate::ThreadInterfacePrivate(ThreadInterface* pp)
    : p_ptr(pp)
    , statusProperty(ThreadInterface::Status::InactiveOrJoined)
    , exitCodeProperty(0)
{
}

/******************************************************************************
 *
 */
ThreadInterface::ThreadInterface()
    : ThreadInterface(pimpl::make_d_ptr<ThreadInterfacePrivate>(this))
{
}

ThreadInterface::ThreadInterface(pimpl::d_ptr_type<ThreadInterfacePrivate> dd)
    : d_ptr(std::move(dd))
    , status(*this, StaticMetaClass::StatusPropertyType, d_ptr->statusProperty)
    , exitCode(*this, StaticMetaClass::ExitCodePropertyType, d_ptr->exitCodeProperty)
{
    // The thread needs a new thread data, therefore reset the one it gets from Object.
    m_threadData.reset();
}

ThreadInterface::~ThreadInterface()
{
}

void ThreadInterface::onQuit(Event& event)
{
    if (event.target() != shared_from_this())
    {
        event.setHandled(false);
        return;
    }

    CTRACE(threads, "async exit");
    QuitEvent& evQuit = static_cast<QuitEvent&>(event);
    exit(evQuit.getExitCode());
}

void ThreadInterface::attachToParentThread()
{
    auto td = ThreadData::getThisThreadData();
    if (!td)
    {
        return;
    }
    auto parent = td->thread();
    if (!parent || parent.get() == this)
    {
        return;
    }

    parent->addChild(*this);

    lock_guard lock(*parent);
    auto dParent = ThreadInterfacePrivate::get(*parent);
    dParent->childThreads.push_back(as_shared<ThreadInterface>(shared_from_this()));
}

void ThreadInterface::detachFromParentThread()
{
    if (!getParent())
    {
        return;
    }

    auto parent = as_shared<ThreadInterface>(getParent()->shared_from_this());
    auto self = shared_from_this();
    parent->removeChild(*this);

    lock_guard lock(*parent);
    auto dParent = ThreadInterfacePrivate::get(*parent);
    mox::erase(dParent->childThreads, self);
}

void ThreadInterface::joinChildThreads()
{
    lock_guard lock(*this);

    while (!d_ptr->childThreads.empty())
    {
        auto last = d_ptr->childThreads.back();

        ScopeRelock relock(*this);
        last->exit(0);
        last->join();
    }
}

Object::VisitResult ThreadInterface::moveToThread(ThreadDataSharedPtr)
{
    // Skip the entire thread content. No objects parented to this thread object shall be moved
    // to other threads, or this thread moved to others.
    return VisitResult::ContinueSibling;
}

void ThreadInterface::setThreadData(ThreadDataSharedPtr td)
{
    auto order = td ? TraverseOrder::PreOrder : TraverseOrder::InversePostOrder;
    auto visitor = [thisTD = td.get()](Object& object)
    {
        lock_guard lock(object);
        if (thisTD)
            object.m_threadData = thisTD->shared_from_this();
        else
            object.m_threadData.reset();
        return Object::VisitResult::Continue;
    };
    traverse(visitor, order);
}

void ThreadInterface::setUp()
{
    lock_guard lock(*this);
    D();
    throwIf<ExceptionType::InvalidThreadData>(!m_threadData);
    throwIf<ExceptionType::InvalidThreadStatus>(d->statusProperty != Status::StartingUp);

    // Attach the thread data to the thread's local storage (TLS)
    td::attachToThread(*m_threadData);

    // Create runloop for the thread.
    CTRACE(threads, "Create runloop for the thread");
    d->runLoop = createRunLoopOverride();
    d->runLoop->getDefaultPostEventSource()->attachQueue(d->threadQueue);

    // make sure the thread objects are set to use the thread data
    ScopeRelock re(*this);
    setThreadData(m_threadData);
}

void ThreadInterface::tearDown()
{
    // Join child threads.
    joinChildThreads();

    // The thread data is no longer valid, therefore reset it, and remove from all the objects owned by this thread loop.
    setThreadData(nullptr);

    // Proceed with teardown
    lock_guard locker(*this);

    td::detachFromThread();

//    m_threadData.reset();
    d_ptr->runLoop.reset();
    CTRACE(threads, "Thread really stopped");
}

void ThreadInterface::initialize()
{
    setThreadData(ThreadData::create(*this));
    addEventHandler(EventType::Quit, std::bind(&ThreadInterface::onQuit, this, std::placeholders::_1));
}

ThreadInterfacePtr ThreadInterface::getThisThread()
{
    return ThreadData::getThisThreadData()->thread();
}

void ThreadInterface::addIdleTask(IdleSource::Task idleTask)
{
    lock_guard lock(*this);
    if (!d_ptr->runLoop->isExiting())
    {
        d_ptr->runLoop->getIdleSource()->addIdleTask(std::move(idleTask));
    }
}

bool ThreadInterface::isRunning() const
{
    lock_guard lock(const_cast<ThreadInterface&>(*this));
    return (d_ptr->statusProperty == Status::StartingUp || d_ptr->statusProperty == Status::Running);
}

void ThreadInterface::start()
{
    lock_guard lock(*this);
    D_PTR(ThreadInterfacePrivate);

    switch (d->statusProperty)
    {
        case Status::InactiveOrJoined:
        {
            // set the status to reflect startup preparation completion
            d_ptr->statusProperty = ThreadInterface::Status::StartingUp;

            {
                ScopeRelock re(*this);
                // execute thread implementation specific start routines
                startOverride();

                // attach this thread to the parent thread.
                attachToParentThread();
            }
            break;
        }
        case Status::StartingUp:
            break;
        case Status::Running:
            break;
        case Status::Stopped:
            break;
    }
}

void ThreadInterface::exit(int exitCode)
{
    lock_guard lock(*this);
    D();

    switch (d->statusProperty)
    {
        case Status::InactiveOrJoined:
        case Status::Stopped:
            CTRACE(threads, "The thread is not running.");
            return;
        case Status::StartingUp:
        {
            ScopeRelock re(*this);
            // post the quit event, execute a delayed exit
            postEvent<QuitEvent>(shared_from_this(), exitCode);
            break;
        }
        case Status::Running:
        {
            CTRACE(threads, "The thread is running. Stop its runloop.");
            d->exitCodeProperty = exitCode;
            quitOverride();
            if (d->runLoop)
            {
                d->runLoop->quit();
            }
            break;
        }
    }

    // Exit child threads.
    auto childExit = [](auto& childThread)
    {
        childThread->exit(0);
    };
    for_each(d->childThreads, childExit);
}

void ThreadInterface::join()
{
    if (status == Status::InactiveOrJoined)
    {
        // The thread was already joined, exit.
        return;
    }
    throwIf<ExceptionType::AttempThreadJoinWithin>(ThreadData::getThisThreadData() == threadData());

    joinChildThreads();
    CTRACE(threads, "Joining thread");
    joinOverride();

    auto keepAlive = shared_from_this();
    detachFromParentThread();

    auto d = ThreadInterfacePrivate::get(*this);
    d->statusProperty = Status::InactiveOrJoined;
    CTRACE(threads, "Thread joined with success");
}

/******************************************************************************
 *
 */
bool postEvent(EventPtr event)
{
    auto target = event->target();
    FATAL(target, "Cannot post event without target");

    auto td = target->threadData();
    if (!td)
    {
        CWARN(event, "target is not in a thread");
        return false;
    }
    auto thread = td->thread();
    FATAL(thread, "No thread!");
    if (!thread)
    {
        return false;
    }
    lock_guard lock(*thread);
    auto d = ThreadInterfacePrivate::get(*thread);
    d->threadQueue.push(std::move(event));

    if (!d->runLoop)
    {
        CTRACE(event, "RunLoop not specified yet.");
        return false;
    }
    CTRACE(event, "Event posted, wake up runloop");
    d->runLoop->scheduleSources();
    return true;
}

}
