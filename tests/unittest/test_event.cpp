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

using namespace mox;

class CustomEvent : public Event
{
public:
    static inline EventType const customEventType = Event::registerNewType(EventPriority::Urgent);
    explicit CustomEvent(ObjectSharedPtr handler)
        : Event(handler, customEventType)
    {}
};

TEST(Event, test_event_api)
{
    ObjectSharedPtr handler = Object::create();
    Event event(handler, BaseEvent);

    EXPECT_EQ(EventId::Base, event.type());
    EXPECT_EQ(EventPriority::Normal, event.priority());
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
    Event eventHi(handler, {EventId::Base, EventPriority::Urgent});
    EXPECT_EQ(EventId::Base, eventHi.type());
    EXPECT_EQ(EventPriority::Urgent, eventHi.priority());

    Event eventLo(handler, {EventId::Base, EventPriority::Low});
    EXPECT_EQ(EventId::Base, eventLo.type());
    EXPECT_EQ(EventPriority::Low, eventLo.priority());
}

TEST(Event, test_register_custom_event_type)
{
    EventType newType = Event::registerNewType();
    EXPECT_LT(EventId::UserType, newType.first);
}

TEST(Event, test_custom_event)
{
    ObjectSharedPtr handler = Object::create();
    EventPtr event = make_event<CustomEvent>(handler);

    EXPECT_LT(EventId::UserType, event->type());
    EXPECT_EQ(CustomEvent::customEventType.first, event->type());
    EXPECT_EQ(EventPriority::Urgent, event->priority());
}
