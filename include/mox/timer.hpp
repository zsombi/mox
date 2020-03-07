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

#ifndef TIMER_HPP
#define TIMER_HPP

#include <mox/meta/signal/signal.hpp>
#include <mox/event_handling/run_loop.hpp>
#include <mox/metainfo/metaclass.hpp>

namespace mox
{

class Timer;
using TimerPtr = std::shared_ptr<Timer>;

/// The Timer class provides coarse timer functionality in Mox. You can create single-shot
/// timers using createSingleShot(), or singleShot() factory methods, and repeating timers
/// using createRepeating() or repeating() methods.
///
/// When the timer expires, the expired signal is emitted.
class MOX_API Timer : public MetaBase, public TimerSource::TimerRecord
{
public:
    /// Expired signal emitted when the timer expires.
    Signal expired{*this, StaticMetaClass::SigExpired};

    /// Specifies the type of the timer.
    enum class Type
    {
        /// Identifies a single shot timer.
        SingleShot,
        /// Identifies a repeating timer.
        Repeating
    };

    /// The timer clock used in the timer.
    using TimerClass = std::chrono::system_clock;

    /// Creates a single shot timer with a \a timeout.
    static TimerPtr createSingleShot(std::chrono::milliseconds timeout);
    /// Creates a repeating timer with an \a interval.
    static TimerPtr createRepeating(std::chrono::milliseconds interval);

    /// Convenience template function, creates a singleton timer with \a timeout, and connects
    /// the \a slot to the timer that is invoked when the timer expires.
    /// \param timeout The timeout when the timer expires.
    /// \param slot The slot to connect to the timer's expired signal.
    /// \return The pair of the timer and the connection objects.
    template <typename Slot>
    static std::pair<TimerPtr, Signal::ConnectionSharedPtr> singleShot(std::chrono::milliseconds timeout, Slot slot)
    {
        auto timer = createSingleShot(timeout);
        auto connection = timer->expired.connect(slot);
        return std::make_pair(timer, connection);
    }

    /// Convenience template function, creates a repeating timer with \a timeout period, and
    /// connects the \a slot to the timer that is invoked when the timer expires.
    /// \param interval The interval of the repeating timer.
    /// \param slot The slot to connect to the timer's expired signal.
    /// \return The pair of the timer and the connection objects.
    template <typename Slot>
    static std::pair<TimerPtr, Signal::ConnectionSharedPtr> repeating(std::chrono::milliseconds interval, Slot slot)
    {
        auto timer = createRepeating(interval);
        auto connection = timer->expired.connect(slot);
        return std::make_pair(timer, connection);
    }

    /// Starts the timer.
    void start();

    /// Returns the type of the timer.
    Type type() const
    {
        return isSingleShot() ? Type::SingleShot : Type::Repeating;
    }
    /// Returns the running state of the timer.
    bool isRunning() const
    {
        return m_isRunning;
    }
    /// Returns the interval for the timer.
    std::chrono::milliseconds interval() const
    {
        return m_interval;
    }

    /// Returns the timer event source this timer uses to track the timeout
    TimerSourcePtr getSource() const
    {
        return m_source;
    }

    MetaInfo(Timer)
    {
        /// Expired signal descriptor. The signal's argument contains the timer object that is
        /// expired when the signal is emitted.
        static inline MetaSignal<Timer, Timer*> SigExpired{"expired"};
    };

private:
    /// Constructor.
    explicit Timer(Type type, std::chrono::milliseconds interval);
    DISABLE_COPY(Timer)
    DISABLE_MOVE(Timer)

    void signal() override;
};

} // mox

#endif // TIMER_HPP
