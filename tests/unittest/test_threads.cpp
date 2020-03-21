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

#include <mox/core/module/thread_loop.hpp>
#include <mox/core/object.hpp>
#include "test_framework.h"

static const mox::EventType evQuit = mox::Event::registerNewType();

class Quitter : public mox::Object
{
public:

    static std::shared_ptr<Quitter> create(mox::Object* parent = nullptr)
    {
        return createObject(new Quitter, parent);
    }

    void quit()
    {
        TRACE("Stop main thread");
        threadData()->thread()->exit(10);
    }

    MetaInfo(Quitter, mox::Object)
    {
        static inline MetaMethod<Quitter> quit{&Quitter::quit, "quit"};
    };
};

class Threads : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();

        mox::registerMetaClass<Quitter>();
    }
};

TEST_F(Threads, test_thread_basics)
{
    TestApp app;

    auto test = mox::ThreadLoop::create();
    test->start();

    EXPECT_NE(test->threadData(), mox::ThreadData::thisThreadData());
    EXPECT_TRUE(test->isRunning());

    // event handler to stop the thread
    auto exiter = [](mox::Event&)
    {
        mox::ThreadLoop::thisThread()->exit(0);
    };
    test->addEventHandler(mox::EventType::Base, exiter);

    // exit wait
    Notifier ping;
    auto wait = ping.get_future();
    auto onStopped = [&ping]()
    {
        ping.set_value();
    };
    test->stopped.connect(onStopped);

    // Post a message to the thread to quit the thread
    EXPECT_TRUE(mox::ThreadLoop::postEvent<mox::Event>(test, mox::EventType::Base));

    test->join();
    wait.wait();
    EXPECT_EQ(mox::ThreadLoop::Status::PostMortem, test->getStatus());
    app.runOnce();
}

TEST_F(Threads, test_parented_thread_deletes_before_quiting)
{
    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();
    {
        TestApp mainThread;

        {
            auto thread = TestThreadLoop::create(std::move(notifyDeath), mainThread.getRootObject().get());
            thread->start();
        }
        EXPECT_EQ(1, TestThreadLoop::threadCount);
        mainThread.runOnce();
    }
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST_F(Threads, test_parented_detached_thread_deletes_before_quiting)
{
    TestApp app;

    Notifier notify;
    Watcher notifyWait = notify.get_future();

    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath), app.getRootObject().get());
        auto slot = [&notify]()
        {
            notify.set_value();
        };
        thread->stopped.connect(slot);
        thread->start();
    }
    EXPECT_EQ(1, TestThreadLoop::threadCount);
    app.runOnce();
    notifyWait.wait();
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST_F(Threads, test_quit_application_from_thread_kills_thread)
{
#if MOX_HOST_LINUX
    GTEST_SKIP_("Flaky on Linux");
#endif
    TestApp app;
    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        auto onEvQuit = [](auto&)
        {
            mox::Application::instance().quit();
        };
        thread->addEventHandler(evQuit, onEvQuit);

        auto onIdle = [thread]()
        {
            mox::ThreadLoop::postEvent<mox::Event>(thread, evQuit);
            return true;
        };
        app.addIdleTask(onIdle);

        thread->start();
    }

    app.run();
    watchDeath.wait();
}

TEST_F(Threads, test_threads2)
{
    mox::Application mainThread;

    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();

    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::thisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        // Add 2 child objects to thread
        mox::Object::create(thread.get());
        auto c2 = mox::Object::create(thread.get());
        mox::Object::create(c2.get());

        thread->start();

        auto mainExit = []()
        {
            mox::Application::instance().exit(101);
        };
        thread->stopped.connect(mainExit);

        auto onIdle = [thread]()
        {
            mox::ThreadLoop::postEvent<mox::Event>(thread, evQuit);
            return true;
        };
        mainThread.addIdleTask(onIdle);
    }

    EXPECT_EQ(101, mainThread.run());
    EXPECT_EQ(0, TestThreadLoop::threadCount);
    watchDeath.wait();
}

TEST_F(Threads, test_signal_connected_to_different_thread)
{
#if MOX_HOST_LINUX
    GTEST_SKIP_("Flaky on Linux");
#endif
    mox::Application mainThread;
    mainThread.setRootObject(*Quitter::create());

    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        EXPECT_NOT_NULL(thread->stopped.connect(*mainThread.castRootObject<Quitter>(), &Quitter::quit));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::thisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        thread->start();
        EXPECT_EQ(1, TestThreadLoop::threadCount);

        auto onIdle = [thread]()
        {
            mox::ThreadLoop::postEvent<mox::Event>(thread, evQuit);
            return true;
        };
        mainThread.addIdleTask(onIdle);
    }

    EXPECT_EQ(10, mainThread.run());
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST_F(Threads, test_signal_connected_to_metamethod_in_different_thread)
{
#if MOX_HOST_LINUX
    GTEST_SKIP_("Flaky on Linux");
#endif
    mox::Application mainThread;
    mainThread.setRootObject(*Quitter::create());

    Notifier notifyDeath;
    Watcher watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        EXPECT_NOT_NULL(mox::metainfo::connect(thread->stopped, *mainThread.castRootObject<Quitter>(), Quitter::StaticMetaClass::quit));

        thread->start();
        EXPECT_EQ(1, TestThreadLoop::threadCount);

        auto onIdle = [thread]()
        {
            mox::ThreadLoop::postEvent<mox::QuitEvent>(thread);
            return true;
        };
        mainThread.addIdleTask(onIdle);
    }

    EXPECT_EQ(10, mainThread.run());
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}
