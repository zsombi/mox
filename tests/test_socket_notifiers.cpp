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
#include <mox/event_handling/socket_notifier.hpp>
#include <mox/timer.hpp>
#include <mox/object.hpp>

#include <iostream>

using namespace mox;

TEST(SocketNotifier, test_stdout_write_watch)
{
    Application test;
    SocketNotifierSharedPtr notifier = SocketNotifier::create(fileno(stdout), SocketNotifier::Modes::Write);

    bool notified = false;
    auto write = [&notified]()
    {
        notified = true;
        ThreadData::thisThreadData()->eventDispatcher()->stop();
    };
    notifier->activated.connect(write);

    // idle task to hit the stdin
    auto idle = []()
    {
        std::cout << "Feed chars to stdout" << std::endl;
        return false;
    };
    ThreadData::thisThreadData()->eventDispatcher()->addIdleTask(idle);
    test.run();
    EXPECT_TRUE(notified);
}
