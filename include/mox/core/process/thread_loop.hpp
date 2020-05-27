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

#ifndef THREAD_LOOP_HPP
#define THREAD_LOOP_HPP

#include <mox/core/process/thread_interface.hpp>
#include <mox/utils/log/logger.hpp>
#include <future>

namespace mox
{

class ThreadLoop;
using ThreadLoopPtr = std::shared_ptr<ThreadLoop>;

class ThreadLoopPrivate;
/// The ThreadLoop class provides event loop functionality on threads. Use this class to
/// create threads that have event dispatching. Objects parented to this instance, as well
/// as the objects created during the lifetime of the thread are owned by the thread.
///
/// Note that despite the thread handler object is owned by the thread, you can use it from
/// outside the thread to communicate with the thread, and for example to join the thread.
/// You cannot join the thread within the thread.
class MOX_API ThreadLoop : public ThreadInterface
{
    DECLARE_PRIVATE(ThreadLoopPrivate);
public:
    /// Exit a running thread, and blocks the current thread till the other thread is completed.
    /// You must call the method from outside of the thread you want to exit.
    /// \param exitCode The exit code.
    void exitAndJoin(int exitCode = 0);

    /// Creates a thread object. You can start the thread by calling start() method.
    /// \return The thread object.
    static ThreadLoopPtr create();

protected:
    /// Constructor.
    explicit ThreadLoop();

    RunLoopBasePtr createRunLoopOverride() final;
    void startOverride() final;
    void joinOverride() final;
};

}

#endif // THREAD_LOOP_HPP
