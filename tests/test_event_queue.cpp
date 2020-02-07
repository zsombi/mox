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
#include <mox/event_handling/event.hpp>
#include <mox/event_handling/event_queue.hpp>

using namespace mox;

TEST(EventQueue, test_queue_api)
{
    EventQueue queue;

    EXPECT_TRUE(queue.empty());

    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(handler, EventType::Base));
    EXPECT_EQ(1u, queue.size());

    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(EventQueue, test_process_events_with_same_priority)
{
    EventQueue queue;
    ObjectSharedPtr handler = Object::create();

    queue.push(make_event<Event>(handler, EventType::Base));
    queue.push(make_event<Event>(handler, EventType::UserType));

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

    queue.push(make_event<Event>(handler, EventType::Base));
    queue.push(make_event<Event>(handler, EventType::UserType, Event::Priority::Urgent));

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
