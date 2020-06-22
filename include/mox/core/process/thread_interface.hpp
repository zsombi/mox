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

#ifndef THREAD_INTERFACE_HPP
#define THREAD_INTERFACE_HPP

#include <mox/config/platform_config.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/core/object.hpp>
#include <future>

namespace mox
{

using ThreadPromise = std::promise<void>;
using ThreadFuture = std::future<void>;

class ThreadInterfacePrivate;
/// ThreadInterface is the base class for the threads with event dispatching capabilities in Mox.
class MOX_API ThreadInterface : public Object
{
protected:
    DECLARE_PRIVATE_PTR(ThreadInterface);

public:
    /// The thread status.
    enum class Status
    {
        /// The thread is inactive, or represents a joined thread. The thread can be re-started.
        InactiveOrJoined,
        /// The thread is started, but the event loop is not running yet.
        StartingUp,
        /// The thread's event loop is running.
        Running,
        /// The thread is stopped, but not joined yet.
        Stopped
    };

    /// \name Metadata
    /// \{
    /// Signal emitted when the thread starts the event loop.
    Signal<ThreadInterface*> started{*this};
    /// Signal emitted right before the thread stops its execution.
    Signal<ThreadInterface*> stopped{*this};

    /// Read-only property reporting the status of the thread.
    using ThreadStatus = StatusProperty<Status>;
    ThreadStatus status;
    /// Read-only property reporting the exit code of the thread.
    using ExitCode = StatusProperty<int>;
    ExitCode exitCode;
    /// \}

    /// Destructor.
    ~ThreadInterface() override;

    /// Returns the current thread's object.
    /// \return The thread object.
    static ThreadInterfacePtr getThisThread();

    /// Adds an idle task to the application run loop.
    /// \param idle The idle function. The run loop takes the ownership over the idle function.
    void onIdle(IdleFunction&& idle);

    /// Returns the running state of the thread.
    /// \return If the thread is running, returns \e true, otherwise \e false.
    bool isRunning() const;

    /// Starts the thread. If the thread object has no parent set, the thread gets automatically
    /// parented to the application's root object at latest when started.
    void start();

    /// Exit a running thread. The thread's loop is stopped and the exit code is passed.
    /// You can call this method both from within and or from outside of the thread.
    /// \param exitCode The exit code of the thread.
    void exit(int exitCode = 0);

    /// Create and initialize a thread of \a DerivedThread, and initializes the thread.
    /// \tparam DerivedThread The thread type derived from ThreadInterface.
    /// \param threadInstance The thread instance to create.
    /// \return The shared object with the thread.
    template <class DerivedThread>
    static auto make_thread(DerivedThread* threadInstance)
    {
        auto thread = std::shared_ptr<DerivedThread>(threadInstance);
        std::static_pointer_cast<ThreadInterface>(thread)->initialize();
        return thread;
    }

protected:
    /// Constructor.
    explicit ThreadInterface();
    /// Constructor with private override.
    explicit ThreadInterface(pimpl::d_ptr_type<ThreadInterfacePrivate> dd);

    /// Override Object::moveToThread() to guard moving objects parented to this thread object
    /// to other thread.
    Object::VisitResult moveToThread(ThreadDataSharedPtr) override;

    /// Move the thread and its child objects to this thread. If \a td is \e nullptr, resets
    /// all object's thread data.
    void setThreadData(ThreadDataSharedPtr td);

    /// Set up the thread. Call this from the thread's main function.
    void setUp();
    /// Cleans up the thread on exit. Call this from the thread's main function.
    void tearDown();

    /// Initializes the thread. You can override this method to provide additional initialization
    /// for your threads.
    virtual void initialize();
    /// Create the runloop of the thread.
    virtual RunLoopBasePtr createRunLoopOverride() = 0;
    /// Start override for the thread specific startup.
    virtual void startOverride() = 0;
    /// Join override for the thread specific join implementations.
    virtual void joinOverride() {}
    /// Quit override for the thread specific quit functions.
    virtual void quitOverride() {}

private:
    // Quit event handler.
    void onQuit(Event&);
};


/// Post an event to the event target thread's run loop.
/// \param event The event to post.
/// \return If the event is posted with success, returns \e true, otherwise \e false.
MOX_API bool postEvent(EventPtr event);

/// Template function, creates an event and posts to \a target.
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

}

DECLARE_LOG_CATEGORY(threads)

#endif // THREAD_INTERFACE_HPP
