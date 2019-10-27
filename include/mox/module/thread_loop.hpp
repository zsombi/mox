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

#include <mox/module/module.hpp>
#include <mox/object.hpp>
#include <mox/event_handling/event_loop.hpp>

#include <mox/signal/signal.hpp>

#include <mox/config/thread.hpp>

#include <future>

namespace mox
{

class ThreadLoop;
using ThreadLoopSharedPtr = std::shared_ptr<ThreadLoop>;

/// The ThreadLoop class provides event loop functionality on threads. Use this class to
/// create threads that have event dispatching. Objects parented to this instance, as well
/// as the objects created during the lifetime of the thread are owned by the thread.
class MOX_API ThreadLoop : public Object, public Module
{
    static inline SignalDescriptor<ThreadLoop*> StartedSignalDescriptor;
    static inline SignalDescriptor<ThreadLoop*> StoppedSignalDescriptor;
public:
    /// The thread status.
    enum class Status
    {
        /// The thread is inactive, its loop has not been started yet.
        Inactive,
        /// The thread's event loop is running.
        Running,
        /// The thread is stopped.
        Stopped
    };
    /// Destructor.
    ~ThreadLoop() override;

    /// Return the status of the thread loop.
    Status getStatus() const;

    /// Starts the thread.
    ThreadLoop& start();

    /// Exit a running thread. The thread's loop is stopped and the exit code is passed.
    /// \param exitCode The exit code of the thread.
    void exit(int exitCode = 0);

    /// Exit a running thread, and blocks the current thread till the other thread is completed.
    /// You must call the method from outside of the thread you want to exit.
    /// \param exitCode The exit code.
    void exitAndJoin(int exitCode = 0);

    /// Creates a thread object. You can start the thread by calling start() method.
    /// \param parent The parent object to which this thread is parented, nullptr if the thread
    /// is not parented.
    /// \return The thread object.
    static ThreadLoopSharedPtr create(Object* parent = nullptr);

    /// Returns the current thread's object.
    /// \return The thread object.
    static ThreadLoopSharedPtr thisThread();

    /// Return the thread's handler. You can use this to join the thread, to detach the thread,
    /// or to perform additional tweaks on the thread.
    /// \return The thread handler.
    std::thread& handler() const
    {
        return m_thread;
    }

    /// \name Metadata
    /// \{
    /// The static metaclass of the thread loop.
    struct MOX_API StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, ThreadLoop, Object>
    {
        Signal started{*this, StartedSignalDescriptor, "started"};
        Signal stopped{*this, StoppedSignalDescriptor, "stopped"};
    };

    /// Signal emitted when the thread starts the event loop.
    Signal started{*this, StartedSignalDescriptor};
    /// Signal emitted right before the thread stops its execution.
    Signal stopped{*this, StoppedSignalDescriptor};
    /// \{

protected:
    /// Constructor.
    explicit ThreadLoop();
    /// Registers the module.
    void registerModule() override;

    /// Runs the thread loop.
    virtual void run();

    /// Override Object::moveToThread() to guard moving objects parented to this thread object
    /// to other thread.
    Object::VisitResult moveToThread(ThreadDataSharedPtr threadData) override;

private:
    void moveToThread();

    void threadMain(std::promise<void> notifier);

    atomic<Status> m_status;
    mutable std::thread m_thread;
};

}

#endif // THREAD_LOOP_HPP
