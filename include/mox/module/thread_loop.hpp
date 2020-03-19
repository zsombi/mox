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
#include <mox/meta/signal/signal.hpp>
#include <mox/config/thread.hpp>
#include <mox/utils/log/logger.hpp>

#include <future>

namespace mox
{

class ThreadLoop;
using ThreadLoopSharedPtr = std::shared_ptr<ThreadLoop>;

/// The ThreadLoop class provides event loop functionality on threads. Use this class to
/// create threads that have event dispatching. Objects parented to this instance, as well
/// as the objects created during the lifetime of the thread are owned by the thread.
///
/// Note that despite the thread handler object is owned by the thread, you can use it from
/// outside the thread to communicate with the thread, and for example to join the thread.
/// You cannot join the thread within the thread.
class MOX_API ThreadLoop : public Object, public Module
{
public:
    /// The thread status.
    enum class Status
    {
        /// The thread is inactive, its loop has not been started yet.
        Inactive,
        /// The thread is started, but the event loop is not running yet.
        StartingUp,
        /// The thread's event loop is running.
        Running,
        /// The thread is stopped.
        Stopped,
        /// The thread is deferred, it is in a post-joined state.
        PostMortem
    };

    /// Return the status of the thread loop.
    Status getStatus() const;

    /// Starts the thread. If the thread object has no parent set, the thread gets automatically
    /// parented to the application's root object at latest when started.
    virtual void start();

    /// Exit a running thread. The thread's loop is stopped and the exit code is passed.
    /// You can call this method both from within and or from outside of the thread.
    /// \param exitCode The exit code of the thread.
    void exit(int exitCode = 0);

    /// Joins the thread. The call completes when the thread is finished. You must call this
    /// function from outside of the running thread this thread object handles. Calling this
    /// method from within the running thread throws Exception(ExceptionType::AttempThreadJoinWithin).
    void join();

    /// Returns the running state of the thread.
    /// \return If the thread is running, returns \e true, otherwise \e false.
    bool isRunning() const;

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

    /// Adds an idle task to the current thread.
    /// \param idleTask The idle task to add.
    static void addIdleTask(RunLoop::IdleFunction idleTask);
    /// Post an event to the current thread's run loop.
    /// \param event The event to post.
    /// \return If the event is posted with success, returns \e true, otherwise \e false.
    static bool postEvent(EventPtr event);

    /// Template function, creates an event and posts to \a target
    /// \tparam EventClass The event class type.
    /// \tparam TargetPtr The target pointer type.
    /// \tparam Arguments The argument types.
    /// \param target The target of the event.
    /// \param args The variadic arguments passed.
    /// \return If the event is posted with success, returns \e true, otherwise \e false.
    template <class EventClass, typename TargetPtr, typename... Arguments>
    static bool postEvent(TargetPtr target, Arguments&&... args)
    {
        auto event = make_event<EventClass>(target, std::forward<Arguments>(args)...);
        return postEvent(std::move(event));
    }

    /// \name Metadata
    /// \{
    /// The static metaclass of the thread loop.
    MetaInfo(ThreadLoop, Object)
    {
        static inline MetaSignal<ThreadLoop, ThreadLoop*> StartedSignalType{"started"};
        static inline MetaSignal<ThreadLoop, ThreadLoop*> StoppedSignalType{"stopped"};
    };

    /// Signal emitted when the thread starts the event loop.
    Signal started{*this, StaticMetaClass::StartedSignalType};
    /// Signal emitted right before the thread stops its execution.
    Signal stopped{*this, StaticMetaClass::StoppedSignalType};
    /// \}

protected:
    /// Constructor.
    explicit ThreadLoop();
    /// Registers the module.
    void registerModule() override;

    /// Prepares the thread for starting.
    void prepare();

    /// Runs the thread loop.
    virtual int run();

    /// Override Object::moveToThread() to guard moving objects parented to this thread object
    /// to other thread.
    Object::VisitResult moveToThread(ThreadDataSharedPtr threadData) override;

    mutable RunLoopSharedPtr m_runLoop;
    mutable std::thread m_thread;
    atomic<Status> m_status = Status::Inactive;
    atomic<int> m_exitCode = 0;

private:
    void moveToThread();
    void quitHandler(Event& event);
    void threadMain(std::promise<void> notifier);

    friend class SocketNotifier;
    friend class Timer;
};

}

DECLARE_LOG_CATEGORY(threads)

#endif // THREAD_LOOP_HPP
