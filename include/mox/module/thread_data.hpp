/*
 * Copyright (C) 2017-2018 bitWelder
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

#ifndef THREAD_DATA_HPP
#define THREAD_DATA_HPP

#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/event_handling/event_queue.hpp>

#include <stack>

namespace mox
{

class ThreadData;
using ThreadDataSharedPtr = std::shared_ptr<ThreadData>;
using ThreadDataWeakPtr = std::weak_ptr<ThreadData>;

class ThreadLoop;
using ThreadLoopSharedPtr = std::shared_ptr<ThreadLoop>;
using ThreadLoopWeakPtr = std::weak_ptr<ThreadLoop>;

/// ThreadData contains the event dispatcher of a thread in Mox. You must have a thread data on each
/// thread that handles events, or has signal-slots connected to other threads.
class MOX_API ThreadData : public std::enable_shared_from_this<ThreadData>
{
public:
    /// Destructor.
    virtual ~ThreadData();

    /// Creates a thread data. Call this from within the thread to create the thread data.
    static ThreadDataSharedPtr create();
    /// Returns the thread data of this thread.
    static ThreadDataSharedPtr thisThreadData();
    /// Returns the main thread data.
    static ThreadDataSharedPtr mainThread();

    /// Returns the event dispatcher of the thread, if any created.
    EventDispatcherSharedPtr eventDispatcher() const;
    /// Returns the topmost event loop of the event dispatcher.
    EventLoopPtr eventLoop() const;

    /// Returns the exit code of the thread.
    int exitCode() const;

    /// Returns the thread object owning the thread data.
    ThreadLoopSharedPtr thread() const;

protected:
    /// Constructor.
    explicit ThreadData();

    /// Sets the exit code of the thread.
    void setExitCode(int code);

    mutable EventQueue m_eventQueue;
    mutable std::stack<EventLoopPtr> m_eventLoopStack;
    mutable ThreadLoopSharedPtr m_thread;
    mutable EventDispatcherSharedPtr m_eventDispatcher;
    atomic<int> m_exitCode;

    friend class ThreadLoop;
    friend class Application;
    friend class EventLoop;
    friend class PostEventSource;
    friend bool postEvent(EventPtr&&);
};

}

#endif // THREAD_DATA_HPP