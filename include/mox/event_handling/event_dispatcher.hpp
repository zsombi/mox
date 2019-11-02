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

#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_sources.hpp>

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

/// EventDispatcher is the entry point to the host operating system providing the event
/// dispatching hooks to handle Mox events. The events are distributed between event sources
/// and the idle task.
///
/// The default event dispatcher has the following event sources:
/// - a default timer source, identified by \c default_timer name,
/// - a default post event source, identified by \c default_post_event name,
/// - a default socket notifier source, identified by \c default_socket_notifier name.
///
/// Idle tasks can be added using \l addIdleTask() method. The idle task is a function
/// of type IdleFunction. An idle task is kept in the dispatcher as long as it returns
/// \e false. Once the task returns \e true, the task is removed from the idle task
/// queue. You must add the task back in order to re-execute it.
class MOX_API EventDispatcher : public std::enable_shared_from_this<EventDispatcher>
{
public:
    /// The idle task function which si run when the dispatcher enters idle state.
    /// \return \e true if the task completed, \e false if not. Non-completed idle tasks are kept
    /// in the idle queue and re-executed on next idle.
    /// \note It is not recommended to have a function that always returns true, as that function
    /// keeps the idle queue busy, which can cause always busy application loop.
    using IdleFunction = std::function<bool()>;

    /// Creates a dispatcher for the current thread, with the default event sources.
    /// \param threadData The ThreadData instance to own the event dispatcher.
    /// \return The created event dispatcher instance.
    static EventDispatcherSharedPtr create(ThreadData& threadData, bool main);
    /// Destructor. Destroys the dispatcher.
    virtual ~EventDispatcher();

    /// Adds an event \a source to the event dispatcher.
    void addEventSource(AbstractEventSourceSharedPtr source);
    /// Looks for an event source in the dispatcher identified by \a name.
    /// \return The event dispatcher with the \a name, \e nullptr if none found.
    AbstractEventSourceSharedPtr findEventSource(std::string_view name);
    /// Looks for an event source in the dispatcher identified by \a name.
    /// \return The event dispatcher with the \a name, \e nullptr if none found.
    AbstractEventSourceSharedPtr findEventSource(std::string_view name) const;

    /// Executes a \a function on all event sources of type \e SourceType. The function is
    /// either a method of SourceType or a function that gets a shared pointer of SourceType
    /// as argument.
    template <class SourceType, typename Function>
    void forEachSource(Function function) const
    {
        for (auto& abstractSource : m_eventSources)
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

    /// Checks whether the event dispatcher is processing the events.
    virtual bool isProcessingEvents() const = 0;
    /// Process the events from the event sources.
    /// \see ProcessFlags
    virtual void processEvents(ProcessFlags flags = ProcessFlags::ProcessAll) = 0;
    /// Stops the current running event dispatcher.
    virtual void stop() = 0;
    /// Wake up a suspended event dispatcher, if the event dispatcher is running, notify the
    /// dispatcher to re-schedule the event sources.
    virtual void wakeUp() = 0;
    /// Returns the number of running timers in the dispatcher.
    virtual size_t runningTimerCount() const = 0;

    /// Adds an idle task to the dispatcher.
    /// \param function The idle task function.
    /// \see IdleFunction
    void addIdleTask(IdleFunction&& function);

protected:
    /// Constructor.
    explicit EventDispatcher(ThreadData& threadData);

    /// Returns the active post-event source.
    PostEventSourcePtr getActivePostEventSource();

    /// Run the idle tasks. Return \e true if there are idle tasks re-scheduled.
    bool runIdleTasks();

    /// Schedule idle tasks.
    virtual void scheduleIdleTasks() = 0;

    /// The address of the ThreadData owning this event dispatcher.
    ThreadData& m_threadData;
    /// Registered event sources.
    std::vector<AbstractEventSourceSharedPtr> m_eventSources;
    /// Idle task queue.
    std::queue<EventDispatcher::IdleFunction> m_idleTasks;

private:

    friend class PostEventSource;
    friend class EventLoop;
};

}

#endif
