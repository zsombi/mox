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

using namespace mox;

class CustomEvent : public Event
{
public:
    static inline EventType const customEventType = Event::registerNewType();
    explicit CustomEvent(ObjectSharedPtr handler)
        : Event(handler, customEventType, Priority::Urgent)
    {}
};

TEST(Event, test_event_api)
{
    ObjectSharedPtr handler = Object::create();
    Event event(handler, EventType::Base);

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
    Event eventHi(handler, EventType::Base, Event::Priority::Urgent);
    EXPECT_EQ(EventType::Base, eventHi.type());
    EXPECT_EQ(Event::Priority::Urgent, eventHi.priority());

    Event eventLo(handler, EventType::Base, Event::Priority::Low);
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
