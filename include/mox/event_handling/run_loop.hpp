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

#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/run_loop_sources.hpp>

#include <mox/event_handling/event_handling_declarations.hpp>

#include <functional>
#include <queue>
#include <type_traits>

namespace mox
{

class ThreadData;
class Timer;
class SocketNotifier;
class EventLoop;

/// RunLoop is the entry point to the host operating system providing the event
/// dispatching hooks to handle Mox events. The events are distributed between event sources
/// and the idle task.
///
/// The default run loop has the following event sources:
/// - a default timer source, identified by \c default_timer name,
/// - a default event source, identified by \c default_post_event name,
/// - a default socket notifier source, identified by \c default_socket_notifier name.
///
/// Idle tasks can be added using \l addIdleTask() method. The idle task is a function
/// of type IdleFunction. An idle task is kept in the run loop as long as it returns
/// \e false. Once the task returns \e true, the task is removed from the idle task
/// queue. You must add the task back in order to re-execute it.
class MOX_API RunLoop : public std::enable_shared_from_this<RunLoop>
{
public:
    /// The idle task function which is run when the run loop enters idle state.
    /// \return \e true if the task completed, \e false if not. Non-completed idle tasks are kept
    /// in the idle queue and re-executed on next idle.
    /// \note It is not recommended to have a function that always returns true, as that function
    /// keeps the idle queue busy, which can cause always busy application loop.
    using IdleFunction = std::function<bool()>;

    /// Creates a run loop for the current thread, with the default event sources.
    /// \param main \e true if the run loop is made for the main thread, \e false if not.
    /// \return The created run loop instance.
    static RunLoopSharedPtr create(bool main);
    /// Destructor. Destroys the run loop.
    virtual ~RunLoop();

    /// \name Event sources
    /// \{
    /// Event sources are modules which inject events from the application layer to the run loop.
    /// Mox defines specialized event sources that serve the desired functionality, and on application
    /// start creates those run lopp sources for each thread. In addition to those, you can create your own
    /// run loop sources deriving them from AbstractRunLoopSource and adding those to the run loop of
    /// the thread.
    ///
    /// Adds a \a source to the run loop.
    void addSource(AbstractRunLoopSourceSharedPtr source);
    /// Looks for a source identified by \a name in the run loop.
    /// \return The source with the \a name, \e nullptr if none found.
    AbstractRunLoopSourceSharedPtr findSource(std::string_view name);
    AbstractRunLoopSourceSharedPtr findSource(std::string_view name) const;

    /// Returns the default timer source.
    TimerSourcePtr getDefaultTimerSource();
    /// Returns the default posted event source.
    EventSourcePtr getDefaultPostEventSource();
    /// Returns the default socket notifier source.
    SocketNotifierSourcePtr getDefaultSocketNotifierSource();

    /// Executes a \a function on all event sources of type \e SourceType. The function is
    /// either a method of SourceType or a function that gets a shared pointer of SourceType
    /// as argument.
    template <class SourceType, typename Function>
    void forEachSource(Function function) const
    {
        for (auto& abstractSource : m_runLoopSources)
        {
            std::shared_ptr<SourceType> source = std::dynamic_pointer_cast<SourceType>(abstractSource);
            if (!source)
            {
                continue;
            }
            if constexpr (std::is_member_function_pointer_v<Function>)
            {
                (source.get()->*function)();
            }
            else
            {
                function(source);
            }
        }
    }
    /// \}

    /// Checks whether the run loop is processing the sources.
    virtual bool isRunning() const = 0;
    /// Process the events from the event sources.
    /// \see ProcessFlags
    virtual void execute(ProcessFlags flags = ProcessFlags::ProcessAll) = 0;
    /// Stops the current running run loop.
    virtual void stopExecution() = 0;
    /// Shuts down the run loop and all its sources.
    virtual void shutDown() = 0;
    /// Wake up a suspended run loop, if the run loop is running, notifies the
    /// run loop to re-schedule the sources.
    virtual void wakeUp() = 0;
    /// Returns the number of running timers in the run loop.
    virtual size_t runningTimerCount() const = 0;

    /// Adds an idle task to the run loop.
    /// \param function The idle task function.
    /// \see IdleFunction
    void addIdleTask(IdleFunction function);

protected:
    /// Constructor.
    explicit RunLoop();

    /// Returns the active event source.
    EventSourcePtr getActiveEventSource();

    /// Run the idle tasks. Return \e true if there are idle tasks re-scheduled.
    bool runIdleTasks();

    /// Schedule idle tasks.
    virtual void scheduleIdleTasks() = 0;

    /// Registered run loop sources.
    std::vector<AbstractRunLoopSourceSharedPtr> m_runLoopSources;
    /// Idle task queue.
    std::queue<RunLoop::IdleFunction> m_idleTasks;

private:

    friend class EventSource;
    friend class EventLoop;
};

}

#endif
