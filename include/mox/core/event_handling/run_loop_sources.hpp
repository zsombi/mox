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

#ifndef RUN_LOOP_SOURCES_HPP
#define RUN_LOOP_SOURCES_HPP

#include <mox/core/event_handling/event.hpp>
#include <mox/core/event_handling/event_handling_declarations.hpp>
#include <mox/core/event_handling/event_queue.hpp>

namespace mox
{

class Timer;

/// Base class for run loop sources.
class MOX_API AbstractRunLoopSource : public std::enable_shared_from_this<AbstractRunLoopSource>
{
public:
    /// Destructor.
    virtual ~AbstractRunLoopSource() = default;
    /// Returns the name of the event source.
    std::string name() const;
    /// Returns the run loop the source is attached to.
    RunLoopBasePtr getRunLoop() const;

    /// Set the run loop to which this event source is attached.
    void attach(RunLoopBase& runLoop);

    /// Detaches the run loop source from the runloop. Resets the source.
    void detach();

    /// The method is called when the run loop event processing starts.
    virtual void initialize(void* data) = 0;

    /// Returns whether the source is functional, meaning it is attached to a runloop and this runloop
    /// is in running state and not exiting.
    bool isFunctional() const;

    /// Notifies the runloop source to re-schedule.
    virtual void wakeUp() {}

protected:
    /// Constructor.
    explicit AbstractRunLoopSource(std::string_view name);

    virtual void detachOverride() = 0;

    RunLoopBaseWeakPtr m_runLoop;
    std::string m_name;
};

/// The TimerSource provides support for single shot and periodic timers in Mox.
/// A run loop can have many timer event sources.
class MOX_API TimerSource : public AbstractRunLoopSource
{
public:
    /// The class defines a timer record. Do not use the class to track timers, but use Timer
    /// class instead.
    class MOX_API TimerRecord : public std::enable_shared_from_this<TimerRecord>
    {
        friend class TimerSource;

    protected:
        /// The event source owning the timer.
        TimerSourcePtr m_source;
        /// The timer interval in milliseconds.
        std::chrono::milliseconds m_interval;
        /// The identifier of the timer.
        int32_t m_id = 0;
        /// The type of the timer.
        bool m_singleShot = true;
        /// The running state of the timer.
        bool m_isRunning = false;

        /// Constructs a timer record with \a interval.
        /// \param interval The interval of the timer.
        /// \param singleShot If the timer is single-shot, \e true. If the timer is repeating, \e false.
        explicit TimerRecord(std::chrono::milliseconds interval, bool singleShot);

    public:
        /// Destructor
        virtual ~TimerRecord();

        /// Signals a timer. You must implement this method to handle timers. If the timer is single-shot,
        /// you must explicitly stop the timer.
        virtual void signal() = 0;
        /// Starts the timer by assigning it to a \a timerSource.
        /// \param timerSource The timer source to own the timer record.
        void start(TimerSource& timerSource);

        /// Stops a timer, and removes it from the timer source.
        void stop();

        /// Returns the type of a timer record.
        bool isSingleShot() const
        {
            return m_singleShot;
        }
        /// Returns the running state of a timer record.
        bool isRunning() const
        {
            return m_isRunning;
        }
        /// Returns the interval of a timer record.
        std::chrono::milliseconds getInterval() const
        {
            return m_interval;
        }
        /// Returns the identifier of the timer.
        int32_t id() const
        {
            return m_id;
        }
    };
    /// The pointer type of a timer record.
    using TimerPtr = std::shared_ptr<TimerRecord>;

    /// Returns the running timer count in the source,
    virtual size_t timerCount() const = 0;

protected:
    /// Constructs the event source.
    explicit TimerSource(std::string_view name);

    /// Adds a timer object to the source.
    virtual void addTimer(TimerRecord& timer) = 0;
    /// Removes a timer object from the source.
    virtual void removeTimer(TimerRecord& timer) = 0;
};

/// This class defines the interface for the event sources. Each event source has an event queue of its own.
class MOX_API EventSource : public AbstractRunLoopSource
{
public:

    /// This is the base class for event dispatchers.
    class MOX_API EventDispatcher
    {
    public:
        /// Destructor.
        virtual ~EventDispatcher() = default;
        /// Dispatches the event.
        virtual void dispatchEvent(Event& event) = 0;
    };

    /// Attaches the event \e queue to the runloop source.
    /// \param queue The event queue to attach.
    void attachQueue(EventQueue& queue);

    /// Dispatches the queued events.
    void dispatchQueuedEvents();

protected:
    /// Constructor.
    explicit EventSource(std::string_view name);

    /// The event queue to process.
    EventQueue* m_eventQueue = nullptr;
};

/// This class defines the interface for the socket notifier event sources.
class MOX_API SocketNotifierSource : public AbstractRunLoopSource
{
public:
    /// This is the notifier class you can use to capture mode changes on a socket handler.
    class MOX_API Notifier : public std::enable_shared_from_this<Notifier>
    {
        friend class SocketNotifierSource;

    public:
        /// The type of the handler.
        typedef int EventTarget;
        /// The event modes.
        enum class Modes
        {
            /// Inactive.
            Inactiv = 0,
            /// Notify on read-ability.
            Read = 0x01,
            /// Notify on write-ability.
            Write = 0x02,
            /// Notify on exception.
            Exception = 0x04,
            /// Notify on error.
            Error = 0x08
        };

        /// Destructor.
        virtual ~Notifier();

        /// Attaches the notifier to a socket notifier source.
        void attach(SocketNotifierSource& source);
        /// Detaches the notifier from the source.
        void detach();

        /// Returns the event modes of this notifier.
        Modes getModes() const
        {
            return m_modes;
        }
        /// Returns the handler watched.
        EventTarget handler() const
        {
            return m_handler;
        }

        /// Signals the notifier about mode change.
        virtual void signal(Modes mode) = 0;

    protected:
        /// Constructor.
        explicit Notifier(EventTarget handler, Modes modes);

        /// The socket notifier event source this notifier connects.
        SocketNotifierSourceWeakPtr m_source;
        /// The socket handler.
        EventTarget m_handler = -1;
        /// The notification modes.
        Modes m_modes = Modes::Read;
    };
    using NotifierPtr = std::shared_ptr<Notifier>;

    /// Returns the notification modes the platform is supporting.
    static Notifier::Modes supportedModes();

protected:
    explicit SocketNotifierSource(std::string_view name);

    /// Add a socket notifier to the event source.
    virtual void addNotifier(Notifier& notifier) = 0;
    /// Remove a socket notifier from the event source.
    virtual void removeNotifier(Notifier& notifier) = 0;
};
ENABLE_ENUM_OPERATORS(SocketNotifierSource::Notifier::Modes)

/// IdleSource defines the idle task handling on a runloop. It is the fourth source created on a runloop.
class MOX_API IdleSource : public AbstractRunLoopSource
{
public:
    /// The idle task function which is run when the idle source is activated.
    /// \return \e true if the task completed, \e false if not. Non-completed idle tasks are kept
    /// in the idle queue and re-executed on next idle.
    /// \note It is not recommended to have a function that always returns true, as that function
    /// keeps the idle queue busy, which can cause always busy application loop.
    using Task = std::function<bool()>;

    /// Adds an idle task to the run loop.
    /// \param function The idle task function.
    /// \see Task
    void addIdleTask(Task task);

protected:
    explicit IdleSource();

    virtual void addIdleTaskOverride(Task&& task) = 0;
};

}

#endif // EVENT_SOURCES_HPP
