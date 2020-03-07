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

#include "test_framework.h"
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_queue.hpp>
#include <mox/event_handling/run_loop.hpp>
#include <mox/event_handling/run_loop_sources.hpp>

using namespace mox;

class TestTimer : public MetaBase, public TimerSource::TimerRecord
{
public:
    static inline SignalTypeDecl<> TestTimerExpiredSignalType;
    Signal expired{*this, TestTimerExpiredSignalType};

    explicit TestTimer(std::chrono::milliseconds interval, bool singleShot)
        : TimerSource::TimerRecord(interval, singleShot)
    {
    }

    void signal() override
    {
        expired();
        if (isSingleShot())
        {
            stop();
        }
    }
};

class TestSocket : public MetaBase, public SocketNotifierSource::Notifier
{
public:
    static inline SignalTypeDecl<Modes> TestSocketSignalType;
    Signal modeChanged{*this, TestSocketSignalType};

    explicit TestSocket(EventTarget handler, Modes modes)
        : SocketNotifierSource::Notifier(handler, modes)
    {
    }

    void signal(Modes mode) override
    {
        modeChanged(mode);
    }
};

struct DispatcherWrapper
{
    RunLoopSharedPtr runLoop;
    TimerSourcePtr timerSource;
    EventSourcePtr postSource;
    SocketNotifierSourcePtr socketSource;
    int exitCode = 0;

    explicit DispatcherWrapper()
        : runLoop(RunLoop::create(true))
        , timerSource(runLoop->getDefaultTimerSource())
        , postSource(runLoop->getDefaultPostEventSource())
        , socketSource(runLoop->getDefaultSocketNotifierSource())
    {
    }
    ~DispatcherWrapper()
    {
        runLoop->shutDown();
    }

    void runOnce()
    {
        auto leave = [this]()
        {
            runLoop->stopExecution();
            return true;
        };
        runLoop->addIdleTask(leave);
        runLoop->execute();
    }
};

TEST(TestEventDispatcher, test_basics)
{
    auto wrapper = DispatcherWrapper();

    auto idleFunc = [&wrapper]()
    {
        wrapper.runLoop->stopExecution();
        wrapper.exitCode = 100;
        return true;
    };
    wrapper.runLoop->addIdleTask(idleFunc);
    wrapper.runLoop->execute();
    EXPECT_EQ(100, wrapper.exitCode);
}

TEST(TestEventDispatcher, test_exit_after_several_idle_calls)
{
    auto wrapper = DispatcherWrapper();
    int count = 5;

    auto idleFunc = [&count, &wrapper]()
    {
        if (--count <= 0)
        {
            wrapper.runLoop->stopExecution();
            wrapper.exitCode = 100;
            return true;
        }
        return false;
    };
    wrapper.runLoop->addIdleTask(idleFunc);
    wrapper.runLoop->execute();
    EXPECT_EQ(100, wrapper.exitCode);
}

TEST(TestEventDispatcher, test_single_shot_timer_quits_loop)
{
    auto wrapper = DispatcherWrapper();
    auto timer = make_polymorphic_shared<TimerSource::TimerRecord, TestTimer>(std::chrono::milliseconds(100), true);

    auto handler = [&wrapper]()
    {
        TRACE("Call exit with 1")
        wrapper.exitCode = 1;
        wrapper.runLoop->stopExecution();
    };
    timer->expired.connect(handler);
    timer->start(*wrapper.timerSource);
    wrapper.runLoop->execute();
    wrapper.runLoop->shutDown();
    EXPECT_EQ(1, wrapper.exitCode);
    EXPECT_EQ(0u, wrapper.runLoop->runningTimerCount());
}

TEST(TestEventDispatcher, test_repeating_timer_quits_loop)
{
    auto wrapper = DispatcherWrapper();
    auto timer = make_polymorphic_shared<TimerSource::TimerRecord, TestTimer>(std::chrono::milliseconds(100), false);

    int repeatCount = 10;
    auto handler = [&repeatCount, &wrapper]()
    {
        if (--repeatCount <= 0)
        {
            wrapper.exitCode = 1;
            wrapper.runLoop->stopExecution();
        }
    };
    timer->expired.connect(handler);
    timer->start(*wrapper.timerSource);
    wrapper.runLoop->execute();
    wrapper.runLoop->shutDown();
    EXPECT_EQ(1, wrapper.exitCode);
    EXPECT_EQ(0u, wrapper.runLoop->runningTimerCount());
}

TEST(TestEventDispatcher, test_ping_timer_idle_task)
{
    auto wrapper = DispatcherWrapper();
    auto ping = make_polymorphic_shared<TimerSource::TimerRecord, TestTimer>(std::chrono::milliseconds(100), false);

    int countDown = 3;
    auto pingHandler = [&countDown, wrapper]()
    {
        if (--countDown <= 0)
        {
            wrapper.runLoop->stopExecution();
            return;
        }
        wrapper.runLoop->wakeUp();
    };
    ping->expired.connect(pingHandler);
    ping->start(*wrapper.timerSource);
    wrapper.runLoop->execute();
    wrapper.runLoop->shutDown();
    EXPECT_EQ(0u, wrapper.runLoop->runningTimerCount());
    EXPECT_EQ(0, countDown);
}

class Filter : public Object
{
    explicit Filter() = default;

    bool filter(Event& event)
    {
        eventFiltered = (event.type() == type);
        return eventFiltered;
    }
public:
    static inline EventType const type = Event::registerNewType();

    static auto create(Object* parent = nullptr)
    {
        auto filter = createObject(new Filter, parent);
        filter->addEventFilter(type, std::bind(&Filter::filter, filter.get(), std::placeholders::_1));
        return filter;
    }

    bool eventFiltered = false;
};

class EventTarget : public Object
{
    explicit EventTarget() = default;

    void process(Event&)
    {
        eventReached = true;
    }
public:
    static std::shared_ptr<EventTarget> create(Object* parent = nullptr)
    {
        auto handler = createObject(new EventTarget, parent);
        handler->addEventHandler(EventType::Base, std::bind(&EventTarget::process, handler.get(), std::placeholders::_1));
        return handler;
    }

    bool eventReached = false;
};



TEST(TestEventDispatcher, test_post_event)
{
    auto wrapper = DispatcherWrapper();
    auto host = Object::create();
    auto quitHandler = [&wrapper](Event& event)
    {
        if (event.type() == EventType::Quit)
        {
            wrapper.exitCode = static_cast<QuitEvent&>(event).getExitCode();
            wrapper.runLoop->stopExecution();
        }
    };

    host->addEventHandler(EventType::Quit, quitHandler);

    wrapper.postSource->push(make_event<Event>(host, EventType::Base));
    wrapper.runLoop->addIdleTask([host, &wrapper]() { wrapper.postSource->push(make_event<QuitEvent>(host, 111)); return true; });
    wrapper.runLoop->execute();
    EXPECT_EQ(111, wrapper.exitCode);
}

TEST(TestEventDispatcher, test_filter_events)
{
    auto wrapper = DispatcherWrapper();
    auto host = Object::create();

    auto filter = [](Event& event)
    {
        return (event.type() == Filter::type);
    };
    host->addEventFilter(Filter::type, filter);
    auto handler = [&wrapper](Event&)
    {
        wrapper.exitCode = 101;
    };
    host->addEventHandler(Filter::type, handler);
    wrapper.postSource->push(make_event<Event>(host, Filter::type));
    wrapper.runOnce();
    EXPECT_NE(101, wrapper.exitCode);
}

TEST(TestEventDispatcher, test_pass_event_filter)
{
    auto wrapper = DispatcherWrapper();
    auto host = Object::create();

    auto filter = [](Event& event)
    {
        return (event.type() == Filter::type);
    };
    host->addEventFilter(Filter::type, filter);
    auto handler = [&wrapper](Event&)
    {
        wrapper.exitCode = 101;
    };
    host->addEventHandler(EventType::Base, handler);
    wrapper.postSource->push(make_event<Event>(host, EventType::Base));
    wrapper.runOnce();
    EXPECT_EQ(101, wrapper.exitCode);
}

TEST(TestEventDispatcher, test_filter_events_from_filter)
{
    auto wrapper = DispatcherWrapper();
    ObjectSharedPtr root = Object::create();
    std::shared_ptr<Filter> filter1 = Filter::create(root.get());
    std::shared_ptr<Filter> filter2 = Filter::create(filter1.get());
    std::shared_ptr<EventTarget> handler = EventTarget::create(filter2.get());

    wrapper.postSource->push(make_event<Event>(handler, Filter::type));

    wrapper.runOnce();
    EXPECT_TRUE(filter1->eventFiltered);
    EXPECT_FALSE(filter2->eventFiltered);
    EXPECT_FALSE(handler->eventReached);
}

TEST(TestEventDispatcher, test_stdout_write_watch)
{
    auto wrapper = DispatcherWrapper();
    auto notifier = make_polymorphic_shared<SocketNotifierSource::Notifier, TestSocket>(fileno(stdout), SocketNotifierSource::Notifier::Modes::Write);

    bool notified = false;
    auto onWrite = [&notified, &wrapper]()
    {
        notified = true;
        wrapper.runLoop->stopExecution();
    };
    notifier->modeChanged.connect(onWrite);
    notifier->attach(*wrapper.socketSource);

    // idle task to hit the stdin
    auto idle = []()
    {
        std::cout << "Feed chars to stdout" << std::endl;
        return true;
    };
    wrapper.runLoop->addIdleTask(idle);
    wrapper.runLoop->execute();
    EXPECT_TRUE(notified);
}


TEST(TestEventDispatcher, test_remove_handler_token_in_event_handling)
{
    auto wrapper = DispatcherWrapper();
    auto object = Object::create();

    int count = 0;
    auto handler = [&count](auto& event)
    {
        ++count;
        event.setHandled(false);
    };
    object->addEventHandler(EventType::UserType, handler);

    auto token = Object::EventTokenPtr();
    auto autoDeleter = [&count, &token](auto& event)
    {
        ++count;
        event.setHandled(false);
        token->erase();
    };
    token = object->addEventHandler(EventType::UserType, autoDeleter);
    EXPECT_EQ(object, std::static_pointer_cast<Object>(token->getTarget()));

    // add two more.
    object->addEventHandler(EventType::UserType, handler);
    object->addEventHandler(EventType::UserType, handler);

    wrapper.postSource->push(make_event<Event>(object, EventType::UserType));
    wrapper.runOnce();

    EXPECT_EQ(4, count);
    EXPECT_EQ(nullptr, token->getTarget());

    count = 0;
    wrapper.postSource->push(make_event<Event>(object, EventType::UserType));
    wrapper.runOnce();
    EXPECT_EQ(3, count);
}

TEST(TestEventDispatcher, test_remove_filter_token_in_event_handling)
{
    auto wrapper = DispatcherWrapper();
    auto object = Object::create();

    int count = 0;
    auto filter = [&count](auto& event)
    {
        ++count;
        event.setHandled(false);
        return false;
    };
    object->addEventFilter(EventType::UserType, filter);

    auto token = Object::EventTokenPtr();
    auto autoDeleter = [&count, &token](auto& event)
    {
        ++count;
        event.setHandled(false);
        token->erase();
        return false;
    };
    token = object->addEventFilter(EventType::UserType, autoDeleter);
    EXPECT_EQ(object, std::static_pointer_cast<Object>(token->getTarget()));

    // add two more.
    object->addEventFilter(EventType::UserType, filter);
    object->addEventFilter(EventType::UserType, filter);

    wrapper.postSource->push(make_event<Event>(object, EventType::UserType));
    wrapper.runOnce();

    EXPECT_EQ(4, count);
    EXPECT_EQ(nullptr, token->getTarget());

    count = 0;
    wrapper.postSource->push(make_event<Event>(object, EventType::UserType));
    wrapper.runOnce();
    EXPECT_EQ(3, count);
}
