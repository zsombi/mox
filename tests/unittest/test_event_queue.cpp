/*
 * Copyright (C) 2017-2020 bitWelder
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
#include <mox/core/event_handling/event.hpp>
#include <mox/core/event_handling/event_queue.hpp>

using namespace mox;

constexpr EventType UserEvent = {EventId::UserType, EventPriority::Normal};
constexpr EventType UserEventHi = {EventId::UserType, EventPriority::Urgent};

class NoCompressEvent : public Event
{
public:
    explicit NoCompressEvent(ObjectSharedPtr target, EventType type)
        : Event(target, type)
    {}
    bool isCompressible() const override
    {
        return false;
    }
};

TEST(EventQueue, test_queue_api)
{
    EventQueue queue;

    EXPECT_TRUE(queue.empty());

    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(handler, BaseEvent));
    EXPECT_EQ(1u, queue.size());

    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(EventQueue, test_push_same_event_type_triggers_compression)
{
    EventQueue queue;
    auto target = Object::create();

    queue.push(make_event<Event>(target, BaseEvent));
    queue.push(make_event<Event>(target, QuitEvent));
    EXPECT_EQ(2u, queue.size());

    queue.push(make_event<Event>(target, BaseEvent));
    EXPECT_EQ(2u, queue.size());
}

TEST(EventQueue, test_push_event_no_compress)
{
    EventQueue queue;
    auto target = Object::create();

    queue.push(make_event<Event>(target, BaseEvent));
    queue.push(make_event<Event>(target, QuitEvent));

    queue.push(make_event<NoCompressEvent>(target, BaseEvent));
    EXPECT_EQ(3u, queue.size());
}

TEST(EventQueue, test_process_events_with_same_priority)
{
    EventQueue queue;
    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(handler, BaseEvent));
    queue.push(make_event<Event>(handler, UserEvent));

    EXPECT_EQ(2u, queue.size());

    int step = 0;
    auto checker = [&step](Event& event) ->bool
    {
        switch (step++)
        {
            case 0:
            {
                EXPECT_EQ(BaseEvent.first, event.type());
                break;
            }
            case 1:
            {
                EXPECT_EQ(EventId::UserType, event.type());
                break;
            }
        }

        return true;
    };
    queue.dispatch(checker);
}

TEST(EventQueue, test_process_event_priority_changes_order)
{
    EventQueue queue;
    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(handler, BaseEvent));
    queue.push(make_event<Event>(handler, UserEventHi));

    EXPECT_EQ(2u, queue.size());

    int step = 0;
    auto checker = [&step](Event& event) ->bool
    {
        switch (step++)
        {
            case 1:
            {
                EXPECT_EQ(BaseEvent.first, event.type());
                break;
            }
            case 0:
            {
                EXPECT_EQ(EventId::UserType, event.type());
                break;
            }
        }

        return true;
    };
    queue.dispatch(checker);
}
