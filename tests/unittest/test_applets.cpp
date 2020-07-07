/*
 * Copyright (C) 2017-2018 bitWelder
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

#include <mox/core/process/applet.hpp>
#include <mox/core/object.hpp>
#include "test_framework.h"

using namespace mox;

static EventType evApplet = Event::registerNewType();

class Applets : public UnitTest
{
public:
    explicit Applets() = default;

    AppletPtr applet;

protected:
    void TearDown() override
    {
        UnitTest::TearDown();
        applet.reset();
    }
};

TEST_F(Applets, test_simple_applet)
{
    TestCoreApp app;

    applet = Applet::create();

    EXPECT_EQ(Applet::Status::InactiveOrJoined, Applet::Status(applet->status));

    applet->start();
    EXPECT_EQ(Applet::Status::Running, Applet::Status(applet->status));

    auto onAppQuit = [&app, this]
    {
        TRACE("exit app main loop, close applet");
        applet->exit();
        app.exit();
        return true;
    };
    app.runOnce(onAppQuit);
    EXPECT_EQ(Applet::Status::InactiveOrJoined, Applet::Status(applet->status));
}

TEST_F(Applets, test_start_applet_twice)
{
    TestCoreApp app;
    applet = Applet::create();

    applet->start();
    EXPECT_EQ(Applet::Status::Running, Applet::Status(applet->status));

    applet->start();
    EXPECT_EQ(Applet::Status::Running, Applet::Status(applet->status));

    // make sure we clean this up.
    applet->exit();
}

TEST_F(Applets, test_exit_without_start)
{
#if defined(MOX_ENABLE_LOGS)
    ScopeLogType<mox::LogType::All> scopeLogs("threads");
    EXPECT_TRACE(threads, "The thread is not running.");
#endif
    TestCoreApp app;
    applet = Applet::create();
    applet->exit();

    applet->start();
    applet->exit();
}

TEST_F(Applets, test_quit_app_when_applet_stops)
{
    TestCoreApp app;
    applet = Applet::create();

    auto onAppletStopped = [&app]()
    {
        app.exit();
    };
    applet->stopped.connect(onAppletStopped);
    auto selfClose = [this](auto&)
    {
        applet->exit(1);
    };
    applet->addEventHandler(evApplet, selfClose);

    postEvent<Event>(applet, evApplet);

    applet->start();
    app.run();
    EXPECT_EQ(1, int(applet->exitCode));
}
