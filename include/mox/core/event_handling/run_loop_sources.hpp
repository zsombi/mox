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
class TimerCore;
using TimerCorePtr = std::shared_ptr<TimerCore>;
using TimerCoreWeakPtr = std::weak_ptr<TimerCore>;

/// The TimerCore provides support for single shot and periodic timers in Mox.
/// A run loop can have many timer sources.
class MOX_API TimerCore : public std::enable_shared_from_this<TimerCore>
{
public:
    /// Destructor
    virtual ~TimerCore();

    /// Signals a timer. You must implement this method to handle timers. If the timer is single-shot,
    /// you must explicitly stop the timer.
    virtual void signal() = 0;

    /// Starts the timer by assigning it to a \a timerSource.
    /// \param timerSource The timer source to own the timer record.
    void start(RunLoopBase& runLoop);

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
protected:
    /// Constructs a timer record with \a interval.
    /// \param interval The interval of the timer.
    /// \param singleShot If the timer is single-shot, \e true. If the timer is repeating, \e false.
    explicit TimerCore(std::chrono::milliseconds interval, bool singleShot);

    RunLoopBaseWeakPtr m_runLoop;
    /// The timer interval in milliseconds.
    std::chrono::milliseconds m_interval;
    /// The identifier of the timer.
    int32_t m_id = 0;
    /// The type of the timer.
    bool m_singleShot = true;
    /// The running state of the timer.
    bool m_isRunning = false;
};

class MOX_API SocketNotifierCore : public std::enable_shared_from_this<SocketNotifierCore>
{
public:
    /// The type of the handler.
    typedef int EventTarget;
    /// The event modes.
    enum class Modes
    {
        /// Inactive.
        Inactive = 0,
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
    virtual ~SocketNotifierCore();

    /// Attaches the notifier to a socket notifier source.
    void attach(RunLoopBase& runLoop);
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
    explicit SocketNotifierCore(EventTarget handler, Modes modes);

    /// The socket notifier event source this notifier connects.
    RunLoopBaseWeakPtr m_runLoop;
    /// The socket handler.
    EventTarget m_handler = -1;
    /// The notification modes.
    Modes m_modes = Modes::Read;
};
ENABLE_ENUM_OPERATORS(SocketNotifierCore::Modes)
using SocketNotifierCorePtr = std::shared_ptr<SocketNotifierCore>;



class MOX_API EventDispatchCore
{
public:
    virtual ~EventDispatchCore() = default;

    virtual void dispatchEvent(Event &event) = 0;
};

}

#endif // EVENT_SOURCES_HPP
