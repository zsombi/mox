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

#include <mox/utils/globals.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_handling_declarations.hpp>
#include <mox/event_handling/event_queue.hpp>

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
    RunLoopSharedPtr getRunLoop() const;

    /// Set the run loop to which this event source is attached.
    virtual void setRunLoop(RunLoop& runLoop);

    /// The method is called when the run loop event processing starts.
    virtual void prepare();

    /// The method is called to clean up the source when the run loop is exiting.
    virtual void clean();

protected:
    /// Constructor.
    explicit AbstractRunLoopSource(std::string_view name);

    RunLoopWeakPtr m_runLoop;
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

    /// Notifies the event source to re-schedule.
    virtual void wakeUp() = 0;

    /// Dispatches the queued events.
    void dispatchQueuedEvents();

    /// Pushes an event to process, and wakes up the run loop source.
    /// \param event The event pushed to the source to process on the next scheduled run loop.
    void push(EventPtr event);

protected:
    /// Constructor.
    explicit EventSource(std::string_view name);

    /// The event queue to process.
    EventQueue m_eventQueue;
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

}

#endif // EVENT_SOURCES_HPP
