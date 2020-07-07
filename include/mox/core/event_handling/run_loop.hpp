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

#ifndef RUN_LOOP_HPP
#define RUN_LOOP_HPP

#include <mox/utils/locks.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <mox/core/event_handling/event.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>

#include <mox/core/event_handling/event_handling_declarations.hpp>

#include <functional>
#include <queue>
#include <type_traits>

namespace mox
{

class Timer;
class SocketNotifier;

/// RunLoopBase is the base class of the runloops in MOX. Contains common API to maintain runloop sources
/// on full runloops and runloop hooks.
class MOX_API RunLoopBase : public std::enable_shared_from_this<RunLoopBase>
{
public:
    enum class Status
    {
        NotStarted,
        Running,
        Exiting,
        Stopped
    };

    using EventProcessingCallback = std::function<void()>;
    using DownCallback = std::function<void()>;
    
    /// Destructor.
    virtual ~RunLoopBase() = default;

    /// Sets the event processing callack invoked every time the runloop is woken up.
    /// \param callback The callback to call when the runloop is woken up. 
    void setEventProcessingCallback(EventProcessingCallback callback);
    
    /// Sets the callback invoked when the runloop goes down.
    void setRunLoopDownCallback(DownCallback callback);

    /// Starts a \p timer and adds it to the runloop.
    /// \param timer The timer to start.
    bool startTimer(TimerCore& timer);
    /// Removes a timer from the runloop.
    /// \param timer The timer to remove.
    void removeTimer(TimerCore& timer);

    /// Attaches a socket notifier to the runloop.
    /// \param notifier The socket notifier to attach.
    bool attachSocketNotifier(SocketNotifierCore& notifier);
    /// Detaches a socket notifier to the runloop.
    /// \param notifier The socket notifier to detach.
    void detachSocketNotifier(SocketNotifierCore& notifier);

    /// Wake up a suspended runloop, and if the runloop is running, notifies the
    /// run loop to re-schedule the sources.
    void scheduleSources();

    /// Quits a running runloop.
    void quit();

    /// Returns the status of a runloop.
    Status getStatus() const;

    /// Adds a function to call on idle.
    /// \param idle The idle function to call.
    /// \see IdleFunction
    void onIdle(IdleFunction&& idle);

protected:
    explicit RunLoopBase() = default;

    /// Notifies about runloop down. After this call, the runloop is no longer usable.
    void notifyRunLoopDown();

    virtual void startTimerOverride(TimerCore& timer) = 0;
    virtual void removeTimerOverride(TimerCore& timer) = 0;
    virtual void attachSocketNotifierOverride(SocketNotifierCore& notifier) = 0;
    virtual void detachSocketNotifierOverride(SocketNotifierCore& notifier) = 0;
    virtual void scheduleSourcesOverride() = 0;
    virtual void stopRunLoop() = 0;
    virtual void onIdleOverride(IdleFunction&& idle) = 0;

    EventProcessingCallback m_processEventsCallback;
    DownCallback m_closedCallback;
    std::atomic<Status> m_status = Status::NotStarted;
};

/// RunLoop is the entry point to the host operating system providing the event
/// dispatching hooks to handle Mox events. The events are distributed between event sources
/// and the idle task.
///
/// The default run loop has the following event sources:
/// - a default timer source, identified by \c default_timer name,
/// - a default event source, identified by \c default_post_event name,
/// - a default socket notifier source, identified by \c default_socket_notifier name.
class MOX_API RunLoop : public RunLoopBase
{
public:
    /// Creates a run loop for the current thread, with the default event sources.
    /// \param main \e true if the run loop is made for the main thread, \e false if not.
    /// \return The created run loop instance.
    static RunLoopPtr create(bool main);

    /// Creates a run loop hook for the current thread, with the default event sources. The application
    /// must have a running loop to which the run loop is attached.
    /// \return The created run loop instance.
    static RunLoopHookPtr createHook();

    /// Destructor. Destroys the run loop.
    ~RunLoop() override;

    /// Process the events from the event sources.
    /// \see ProcessFlags
    virtual void execute(ProcessFlags flags = ProcessFlags::ProcessAll) = 0;

protected:
    /// Constructor.
    explicit RunLoop() = default;
};

class MOX_API RunLoopHook : public RunLoopBase
{
public:
    static RunLoopHookPtr create();

protected:
    explicit RunLoopHook() = default;
};

}

#endif
