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

#include <private/process_p.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

ThreadInterfacePrivate::ThreadInterfacePrivate(ThreadInterface* pp)
    : p_ptr(pp)
{
}

void ThreadInterfacePrivate::attachToParentThread()
{
    P();
    auto td = ThreadData::getThisThreadData();
    if (!td)
    {
        CTRACE(threads, "Cannot attach to parent thread, when the thread data is not set!");
        return;
    }
    auto parent = td->thread();
    if (!parent || parent.get() == p)
    {
        return;
    }

    parent->addChild(*p);

    lock_guard lock(*parent);
    auto dParent = ThreadInterfacePrivate::get(*parent);
    dParent->childThreads.push_back(as_shared<ThreadInterface>(p));
}

void ThreadInterfacePrivate::detachFromParentThread()
{
    P();
//    lock_guard lock(*p);
    if (!p->getParent())
    {
        return;
    }

    auto parent = as_shared<ThreadInterface>(p->getParent());
    auto self = p->shared_from_this();
    parent->removeChild(*p);
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
    , status(*this, d_ptr->statusProperty)
    , exitCode(*this, d_ptr->exitCodeProperty)
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
    QuitEventType& evQuit = static_cast<QuitEventType&>(event);
    exit(evQuit.getExitCode());
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

    // The event dispatcher
    auto dispatcher = [d]()
    {
        auto dispatchEvent = [](auto& event)
        {
            auto dispatcher = std::static_pointer_cast<EventDispatchCore>(event.target());

            if (!dispatcher)
            {
                return;
            }

            dispatcher->dispatchEvent(event);
        };

        CTRACE(event, "process queue with" << d->threadQueue.size() << "events");
        d->threadQueue.dispatch(dispatchEvent);
    };
    d->runLoop->setEventProcessingCallback(dispatcher);

    // make sure the thread objects are set to use the thread data
    ScopeRelock re(*this);
    setThreadData(m_threadData);
}

void ThreadInterface::tearDown()
{
    d_ptr->detachFromParentThread();

    // The thread data is no longer valid, therefore reset it, and remove from all the objects owned by this thread loop.
    setThreadData(nullptr);

    // Proceed with teardown
    {
        lock_guard locker(*this);

        td::detachFromThread();

        d_ptr->runLoop.reset();
        CTRACE(threads, "Thread is joinable");
        d_ptr->statusProperty = Status::InactiveOrJoined;
    }
    // Loop through the child threads and join them all. Once joined, clean the child threads.
    auto joiner = [](auto& thread)
    {
        CTRACE(threads, "Join child thread" << as_shared<ThreadLoop>(thread));
        thread->joinOverride();
    };
    for_each(d_ptr->childThreads, joiner);

    d_ptr->childThreads.clear();
}

void ThreadInterface::initialize()
{
    setThreadData(ThreadData::create(*this));
    addEventHandler(QuitEvent, std::bind(&ThreadInterface::onQuit, this, std::placeholders::_1));
}

ThreadInterfacePtr ThreadInterface::getThisThread()
{
    return ThreadData::getThisThreadData()->thread();
}

void ThreadInterface::onIdle(IdleFunction&& idle)
{
    FATAL(d_ptr->runLoop, "Invalid runloop");
    if (d_ptr->runLoop->getStatus() >= RunLoopBase::Status::Exiting)
    {
        return;
    }
    d_ptr->runLoop->onIdle(std::forward<IdleFunction>(idle));
}

bool ThreadInterface::isRunning() const
{
    lock_guard lock(const_cast<ThreadInterface&>(*this));
    return (d_ptr->statusProperty == Status::StartingUp || d_ptr->statusProperty == Status::Running);
}

void ThreadInterface::start()
{
    D_PTR(ThreadInterfacePrivate);

    switch (d->statusProperty)
    {
        case Status::InactiveOrJoined:
        {
            // set the status to reflect startup preparation completion
            d_ptr->statusProperty = ThreadInterface::Status::StartingUp;
            // attach this thread to the parent thread.
            d_ptr->attachToParentThread();
            // execute thread implementation specific start routines
            startOverride();
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
        {
            CTRACE(threads, "The thread is not running.");
            return;
        }
        case Status::StartingUp:
        {
            // post the quit event, execute a delayed exit
            ScopeRelock re(*this);
            CTRACE(threads, "Post exit on a starting up thread.");
            postEvent<QuitEventType>(shared_from_this(), exitCode);
            break;
        }
        case Status::Running:
        {
            CTRACE(threads, "The thread is running. Stop its runloop.");
            d->exitCodeProperty = exitCode;
            quitOverride();
            if (d->runLoop)
            {
                ScopeRelock re(*this);
                d->runLoop->quit();
            }
            break;
        }
    }

    // Exit child threads.
    auto childExit = [](auto& childThread)
    {
        CTRACE(threads, "Exiting child" << as_shared<ThreadLoop>(childThread));
        childThread->exit(0);
    };
    for_each(d->childThreads, childExit);
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
    if (d->runLoop && d->runLoop->getStatus() >= RunLoopBase::Status::Exiting)
    {
        WARN("Destination thread runloop is exiting. Event" << int(event->type()) << "was not posted!");
        return false;
    }

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
