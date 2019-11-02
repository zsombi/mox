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

    /// Return the status of the thread loop.
    Status getStatus() const;

    /// Starts the thread. If the thread object has no parent set, the thread is started as detached.
    /// \param detached If set to \e true, the thread starts detached. Otherwise starts as joinable.
    /// If the thread object is a parentless object, this argument is ingored.
    virtual void start(bool detached = false);

    /// Exit a running thread. The thread's loop is stopped and the exit code is passed.
    /// You can call this method both from within and or from outside of the thread.
    /// \param exitCode The exit code of the thread.
    void exit(int exitCode = 0);

    /// Joins the thread. The call completes when the thread is finished. If the thread
    /// is detached, and \a force is \e false, the call returns immediately without waiting
    /// for the thread completion. You must call this function from outside of the running
    /// thread this thread object handles. Calling this method from within the running thread
    /// has no effect, and return \e false.
    /// \param force If true, the call waits even if the thread is detached.
    /// \return If the join succeeded, returns \e true. If the join is called from within
    /// the running thread, or if the thread is detached, and you call join not forced,
    /// return \e false.
    bool join(bool force = true);

    /// Returns the running state of the thread.
    /// \return If the thread is running, returns \e true, otherwise \e false.
    bool isRunning() const;

    /// Exit a running thread, and blocks the current thread till the other thread is completed.
    /// You must call the method from outside of the thread you want to exit.
    /// \param exitCode The exit code.
    /// \return If the operation succeeds, returns \e true, otherwise \e false.
    bool exitAndJoin(int exitCode = 0);

    /// Creates a thread object. You can start the thread by calling start() method.
    /// \param parent The parent object to which this thread is parented, nullptr if the thread
    /// is not parented.
    /// \return The thread object.
    static ThreadLoopSharedPtr create(Object* parent = nullptr);

    /// Returns the current thread's object.
    /// \return The thread object.
    static ThreadLoopSharedPtr thisThread();

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
    virtual int run();

    /// Override Object::moveToThread() to guard moving objects parented to this thread object
    /// to other thread.
    Object::VisitResult moveToThread(ThreadDataSharedPtr threadData) override;

    atomic<Status> m_status = Status::Inactive;
    mutable std::thread m_thread;

private:
    void moveToThread();
    void quitHandler(Event& event);
    void threadMain(std::promise<void> notifier);
};

}

#endif // THREAD_LOOP_HPP
