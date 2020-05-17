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
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>

using namespace mox;

struct HookWrapper
{
    EventQueue queue;
    RunLoopHookPtr runLoop;
    TimerSourceWeakPtr timerSource;
    EventSourceWeakPtr postSource;
    SocketNotifierSourceWeakPtr socketSource;
    int exitCode = 0;

    explicit HookWrapper()
        : runLoop(RunLoopHook::create())
        , timerSource(runLoop->getDefaultTimerSource())
        , postSource(runLoop->getDefaultPostEventSource())
        , socketSource(runLoop->getDefaultSocketNotifierSource())
    {
        postSource.lock()->attachQueue(queue);
    }
    ~HookWrapper()
    {
    }
};


TEST(TestRunLoopHooks, test_runloop_hook_stop_before_app_stops)
{
    TestCoreApp coreApp;
    HookWrapper hook;

    bool hookStopped = false;
    auto onIdle = [&hook, &hookStopped]()
    {
        hook.runLoop->quit();
        hook.runLoop.reset();
        hookStopped = true;
        return true;
    };
    hook.runLoop->getIdleSource()->addIdleTask(onIdle);

    auto onHookClosed = [&coreApp]()
    {
        coreApp.exit();
        return true;
    };
    hook.runLoop->setRunLoopDownCallback(onHookClosed);

    coreApp.run();
    EXPECT_TRUE(hookStopped);

    EXPECT_TRUE(hook.timerSource.expired());
    EXPECT_TRUE(hook.postSource.expired());
    EXPECT_TRUE(hook.socketSource.expired());
}

TEST(TestRunLoopHooks, test_runloop_hook_stops_with_app_stop)
{
    TestCoreApp coreApp;
    HookWrapper hook;

    bool hookStopped = false;
    auto onIdle = [&hook, &coreApp]()
    {
        CTRACE(event, "HOOK BAILOUT");
        hook.runLoop->quit();
        coreApp.exit();
        return true;
    };
    hook.runLoop->getIdleSource()->addIdleTask(onIdle);
    auto onHookClosed = [&hookStopped]()
    {
        hookStopped = true;
        return true;
    };
    hook.runLoop->setRunLoopDownCallback(onHookClosed);

    coreApp.run();
    EXPECT_TRUE(hookStopped);

    EXPECT_TRUE(hook.timerSource.expired());
    EXPECT_TRUE(hook.postSource.expired());
    EXPECT_TRUE(hook.socketSource.expired());
}

TEST(TestRunLoopHooks, test_runloop_hook_exiter_drops_all_queued_idles)
{
    TestCoreApp coreApp;
    HookWrapper hook;

    auto idleCalls = int(0);
    auto idle1 = [&idleCalls]() { ++idleCalls; CTRACE(event, "idle1 called"); return true; };
    auto idle2 = [&idleCalls]() { ++idleCalls; CTRACE(event, "idle2 called"); return true; };
    auto idle3 = [&idleCalls]() { CTRACE(event, "idle3 called"); return (++idleCalls >= 5); };
    auto exiter = [&hook]()
    {
        CTRACE(event, "HOOK BAILOUT");
        hook.runLoop->quit();
        return true;
    };
    hook.runLoop->getIdleSource()->addIdleTask(idle1);
    hook.runLoop->getIdleSource()->addIdleTask(idle2);
    hook.runLoop->getIdleSource()->addIdleTask(idle3);
    hook.runLoop->getIdleSource()->addIdleTask(exiter);

    auto onHookClosed = [&coreApp]()
    {
        coreApp.exit();
        return true;
    };
    hook.runLoop->setRunLoopDownCallback(onHookClosed);

    coreApp.run();
    // cannot reschedule idle tasks. Therefore each idle task can run only ones.
    EXPECT_EQ(3, idleCalls);
}

TEST(TestRunLoopHooks, test_attach_runloop_hook_while_runloop_is_running)
{
#if MOX_HOST_LINUX
//    GTEST_SKIP_("TestAppCore::addIdleTask crash on Linux");
#endif
    TestCoreApp coreApp;
    std::unique_ptr<HookWrapper> hook;
    bool exitSuccess = false;

    auto attacher = [&hook, &coreApp, &exitSuccess]()
    {
        hook = std::make_unique<HookWrapper>();
        auto exiter = [&hook, &coreApp, &exitSuccess]()
        {
            hook->runLoop->quit();
            coreApp.exit();
            exitSuccess = true;
            return true;
        };
        coreApp.addIdleTask(exiter);
        return true;
    };

    coreApp.runOnce(attacher);
    EXPECT_TRUE(exitSuccess);
}
