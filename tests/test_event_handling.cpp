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
#include <mox/event_handling/event_handler.hpp>
#include <mox/event_handling/event_queue.hpp>
#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/event_loop.hpp>
#include <mox/timer.hpp>
#include <mox/object.hpp>

using namespace mox;

class CustomEvent : public Event
{
public:
    static inline EventType const customEventType = Event::registerNewType();
    explicit CustomEvent(ObjectSharedPtr handler)
        : Event(customEventType, handler, Priority::Urgent)
    {}
};

TEST(Event, test_event_api)
{
    ObjectSharedPtr handler = Object::create();
    Event event(EventType::Base, handler);

    EXPECT_EQ(EventType::Base, event.type());
    EXPECT_EQ(Event::Priority::Normal, event.priority());
    EXPECT_EQ(handler, event.target());
    EXPECT_FALSE(event.isHandled());

    event.setHandled();
    EXPECT_TRUE(event.isHandled());

    event.setHandled(false);
    EXPECT_FALSE(event.isHandled());
}

TEST(Event, test_event_priority)
{
    ObjectSharedPtr handler = Object::create();
    Event eventHi(EventType::Base, handler, Event::Priority::Urgent);
    EXPECT_EQ(EventType::Base, eventHi.type());
    EXPECT_EQ(Event::Priority::Urgent, eventHi.priority());

    Event eventLo(EventType::Base, handler, Event::Priority::Low);
    EXPECT_EQ(EventType::Base, eventLo.type());
    EXPECT_EQ(Event::Priority::Low, eventLo.priority());
}

TEST(Event, test_register_custom_event_type)
{
    EventType newType = Event::registerNewType();
    EXPECT_GT(newType, EventType::UserType);
}

TEST(Event, test_custom_event)
{
    ObjectSharedPtr handler = Object::create();
    EventPtr event = make_event<CustomEvent>(handler);

    EXPECT_GT(event->type(), EventType::UserType);
    EXPECT_EQ(CustomEvent::customEventType, event->type());
    EXPECT_EQ(Event::Priority::Urgent, event->priority());
}


TEST(EventQueue, test_queue_api)
{
    EventQueue queue;

    EXPECT_TRUE(queue.empty());

    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(EventType::Base, handler));
    EXPECT_EQ(1u, queue.size());

    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(EventQueue, test_process_events_with_same_priority)
{
    EventQueue queue;
    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(EventType::Base, handler));
    queue.push(make_event<Event>(EventType::UserType, handler));

    EXPECT_EQ(2u, queue.size());

    int step = 0;
    auto checker = [&step](Event& event) ->bool
    {
        switch (step++)
        {
            case 0:
            {
                EXPECT_EQ(EventType::Base, event.type());
                break;
            }
            case 1:
            {
                EXPECT_EQ(EventType::UserType, event.type());
                break;
            }
        }

        return true;
    };
    queue.process(checker);
}

TEST(EventQueue, test_process_event_priority_changes_order)
{
    EventQueue queue;
    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(EventType::Base, handler));
    queue.push(make_event<Event>(EventType::UserType, handler, Event::Priority::Urgent));

    EXPECT_EQ(2u, queue.size());

    int step = 0;
    auto checker = [&step](Event& event) ->bool
    {
        switch (step++)
        {
            case 1:
            {
                EXPECT_EQ(EventType::Base, event.type());
                break;
            }
            case 0:
            {
                EXPECT_EQ(EventType::UserType, event.type());
                break;
            }
        }

        return true;
    };
    queue.process(checker);
}


TEST(EventDispatcher, test_basics)
{
    EventDispatcherSharedPtr loop = EventDispatcher::create();

    auto idleFunc = [loop]()
    {
        loop->exit(100);
        return true;
    };
    loop->addIdleTask(idleFunc);

    EXPECT_EQ(100, loop->processEvents());
}

TEST(EventDispatcher, test_exit_after_several_idle_calls)
{
    EventDispatcherSharedPtr loop = EventDispatcher::create();
    int count = 5;

    auto idleFunc = [&count, loop]()
    {
        if (--count <= 0)
        {
            loop->exit(100);
            return true;
        }
        return false;
    };
    loop->addIdleTask(idleFunc);

    EXPECT_EQ(100, loop->processEvents());
}

TEST(EventDispatcher, test_single_shot_timer_quits_loop)
{
    EventDispatcherSharedPtr loop = EventDispatcher::create();

    TimerPtr timer = Timer::createSingleShot(std::chrono::milliseconds(100));

    auto handler = []()
    {
        TRACE("Call exit with 1")
        EventDispatcher::get()->exit(1);
    };
    timer->expired.connect(handler);
    timer->start();
    EXPECT_EQ(1, loop->processEvents());
    EXPECT_EQ(0u, loop->runningTimerCount());
}

TEST(EventDispatcher, test_repeating_timer_quits_loop)
{
    EventDispatcherSharedPtr loop = EventDispatcher::create();

    TimerPtr timer = Timer::createRepeating(std::chrono::milliseconds(100));

    int repeatCount = 10;
    auto handler = [&repeatCount]()
    {
        if (--repeatCount <= 0)
        {
            EventDispatcher::get()->exit(1);
        }
    };
    timer->expired.connect(handler);
    timer->start();
    EXPECT_EQ(1, loop->processEvents());
    EXPECT_EQ(0u, loop->runningTimerCount());
}

TEST(EventDispatcher, test_ping_timer_idle_task)
{
    EventDispatcherSharedPtr loop = EventDispatcher::create();
    TimerPtr ping = Timer::createRepeating(std::chrono::milliseconds(500));

    int countDown = 3;
    auto pingHandler = [&countDown]()
    {
        if (--countDown <= 0)
        {
            EventDispatcher::get()->exit();
            return;
        }
        EventDispatcher::get()->wakeUp();
    };
    ping->expired.connect(pingHandler);
    ping->start();
    EXPECT_EQ(0, loop->processEvents());
    EXPECT_EQ(0u, loop->runningTimerCount());
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

class Handler : public Object
{
    explicit Handler() = default;

    void process(Event&)
    {
        eventReached = true;
    }
public:
    static std::shared_ptr<Handler> create(Object* parent = nullptr)
    {
        auto handler = createObject(new Handler, parent);
        handler->addEventHandler(EventType::Base, std::bind(&Handler::process, handler.get(), std::placeholders::_1));
        return handler;
    }

    bool eventReached = false;
};

class QuitHandler : public Object
{
    explicit QuitHandler() = default;

    void quit(Event&)
    {
        ++handleCount;
        if (handleCount == 2)
        {
            EventDispatcher::get()->exit(10);
        }
    }

public:
    static std::shared_ptr<QuitHandler> create(Object* parent = nullptr)
    {
        auto handler = createObject(new QuitHandler, parent);
        handler->addEventHandler(EventType::Base, std::bind(&QuitHandler::quit, handler.get(), std::placeholders::_1));
        return handler;
    }

    int handleCount = 0;
};

TEST(EventDispatcher, test_post_event)
{
    ObjectSharedPtr host = Object::create();
    EventDispatcherSharedPtr dispatcher = EventDispatcher::create();
    EventLoop loop;

    EXPECT_TRUE(dispatcher->postEvent(make_event<Event>(EventType::Base, host)));
    dispatcher->addIdleTask([dispatcher]() { dispatcher->exit(111); return true; });
    EXPECT_EQ(111, dispatcher->processEvents());
    EXPECT_FALSE(dispatcher->postEvent(make_event<Event>(EventType::Base, host)));
}

TEST(EventDispatcher, test_filter_events)
{
    ObjectSharedPtr root = Object::create();
    std::shared_ptr<Filter> filter = Filter::create(root.get());
    std::shared_ptr<Handler> handler = Handler::create(filter.get());

    EventDispatcherSharedPtr dispatcher = EventDispatcher::create();
    EventLoop loop;
    dispatcher->postEvent(make_event<Event>(Filter::type, handler));

    EXPECT_EQ(0u, dispatcher->processEvents(ProcessFlags::RunOnce));
    EXPECT_TRUE(filter->eventFiltered);
    EXPECT_FALSE(handler->eventReached);
}

TEST(EventDispatcher, test_pass_filter_events)
{
    ObjectSharedPtr root = Object::create();
    std::shared_ptr<Filter> filter = Filter::create(root.get());
    std::shared_ptr<Handler> handler = Handler::create(filter.get());

    EventDispatcherSharedPtr dispatcher = EventDispatcher::create();
    EventLoop loop;
    dispatcher->postEvent(make_event<Event>(EventType::Base, handler));

    EXPECT_EQ(0u, dispatcher->processEvents(ProcessFlags::RunOnce));
    EXPECT_FALSE(filter->eventFiltered);
    EXPECT_TRUE(handler->eventReached);
}

TEST(EventDispatcher, test_filter_events_from_filter)
{
    ObjectSharedPtr root = Object::create();
    std::shared_ptr<Filter> filter1 = Filter::create(root.get());
    std::shared_ptr<Filter> filter2 = Filter::create(filter1.get());
    std::shared_ptr<Handler> handler = Handler::create(filter2.get());

    EventDispatcherSharedPtr dispatcher = EventDispatcher::create();
    EventLoop loop;
    dispatcher->postEvent(make_event<Event>(Filter::type, handler));

    EXPECT_EQ(0u, dispatcher->processEvents(ProcessFlags::RunOnce));
    EXPECT_TRUE(filter1->eventFiltered);
    EXPECT_FALSE(filter2->eventFiltered);
    EXPECT_FALSE(handler->eventReached);
}


TEST(EventLoop, test_loop_in_loop)
{
    auto object = Object::create();
    auto handler = QuitHandler::create(object.get());

    auto dispatcher = EventDispatcher::create();
    EventLoop loop;

    EXPECT_TRUE(dispatcher->postEvent(make_event<Event>(EventType::Base, handler)));
    auto timerHandler = [&handler]()
    {
        {
            EventLoop local;
            auto delayedPost = [&handler]()
            {
                EventDispatcher::postEvent(make_event<Event>(EventType::Base, handler));
            };
            Timer::singleShot(std::chrono::milliseconds(100), delayedPost).first->start();
            local.processEvents();
            local.exit();
        }
    };
    Timer::singleShot(std::chrono::milliseconds(200), timerHandler).first->start();

    EXPECT_EQ(10, dispatcher->processEvents());

    EXPECT_EQ(2, handler->handleCount);
}
