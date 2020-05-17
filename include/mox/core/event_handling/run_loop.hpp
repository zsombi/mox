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
    virtual ~RunLoopBase() = default;

    /// Sets the callback invoked when teh runloop goes down.
    void setRunLoopDownCallback(IdleSource::Task callback);

    /// Runloop sources are modules which inject events from the application layer to the runloop.
    /// Mox defines specialized runloop sources that serve the desired functionality, and on application
    /// start creates those runlopp sources for each thread. In addition to those, you can create your own
    /// runloop sources deriving those from AbstractRunLoopSource and addttaching those to the runloop of
    /// the thread.
    ///

    /// Looks for a source identified by \a name in the run loop.
    /// \return The source with the \a name, \e nullptr if none found.
    AbstractRunLoopSourceSharedPtr findSource(std::string_view name);
    AbstractRunLoopSourceSharedPtr findSource(std::string_view name) const;

    /// Executes a \a function on all runloop sources of type \e SourceType. The function is
    /// either a method of SourceType or a function that gets a shared pointer of SourceType
    /// as argument.
    template <class SourceType, typename Function, typename... Arguments>
    void forEachSource(Function function, Arguments... args) const
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
                (source.get()->*function)(std::forward<Arguments>(args)...);
            }
            else
            {
                function(source);
            }
        }
    }
    /// Returns the default timer source.
    TimerSourcePtr getDefaultTimerSource();
    /// Returns the default posted event source.
    EventSourcePtr getDefaultPostEventSource();
    /// Returns the default socket notifier source.
    SocketNotifierSourcePtr getDefaultSocketNotifierSource();
    /// Returns the idle source of the runloop.
    IdleSourcePtr getIdleSource();

    /// Wake up a suspended runloop, and if the runloop is running, notifies the
    /// run loop to re-schedule the sources.
    void scheduleSources();

    /// Quits a running runloop.
    void quit();

    /// Returns the exiting state of a runloop. A runlop is in exiting state if the quit()
    /// is called.
    bool isExiting() const;
    /// Returns the running state of a runloop.
    bool isRunning() const;

protected:
    explicit RunLoopBase() = default;

    /// Sets up the default runloop sources.
    void setupSources();

    /// Notifies about runloop down. After this call, the runloop is no longer usable.
    void notifyRunLoopDown();

    virtual void initialize() = 0;
    virtual bool isRunningOverride() const = 0;
    virtual void scheduleSourcesOverride() = 0;
    virtual void stopRunLoop() = 0;

    friend class AbstractRunLoopSource;
    friend class IdleSource;

private:
    /// Adds a \a source to the runloop. Called by AbstractRunLoopSource::attach().
    void addSource(AbstractRunLoopSourceSharedPtr source);
    /// Removes a \a source from the run loop. Called by AbstractRunLoopSource::detach().
    void removeSource(AbstractRunLoopSource& source);

    /// Registered run loop sources.
    std::vector<AbstractRunLoopSourceSharedPtr> m_runLoopSources;
    IdleSource::Task m_closedCallback;
    std::atomic_bool m_isExiting = false;
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
    static RunLoopSharedPtr create(bool main);

    /// Creates a run loop hook for the current thread, with the default event sources. The application
    /// must have a running loop to which the run loop is attached.
    /// \return The created run loop instance.
    static RunLoopSharedPtr createHook();

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
