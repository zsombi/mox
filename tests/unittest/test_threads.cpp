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

#include <mox/core/process/thread_loop.hpp>
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
};

class Threads : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
    }
};

TEST_F(Threads, test_thread_basics)
{
    TestApp app;

    auto test = mox::ThreadLoop::create();
    test->start();

    EXPECT_NE(test->threadData(), mox::ThreadData::getThisThreadData());
    EXPECT_TRUE(test->isRunning());

    // event handler to stop the thread
    auto exiter = [](mox::Event&)
    {
        mox::ThreadLoop::getThisThread()->exit(0);
    };
    test->addEventHandler(mox::EventType::Base, exiter);

    // exit wait
    mox::ThreadPromise ping;
    auto wait = ping.get_future();
    auto onStopped = [&ping]()
    {
        ping.set_value();
    };
    test->stopped.connect(onStopped);

    // Post a message to the thread to quit the thread
    EXPECT_TRUE(mox::postEvent<mox::Event>(test, mox::EventType::Base));

    test->join();
    wait.wait();
    EXPECT_EQ(mox::ThreadLoop::Status::InactiveOrJoined, test->status);
    app.runOnce();
}

TEST_F(Threads, test_parent_thread_deletes_before_quiting)
{
    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        TestApp getMainThreadData;

        {
            auto thread = TestThreadLoop::create(std::move(notifyDeath));
            mox::ThreadPromise notifyStart;
            mox::ThreadFuture started = notifyStart.get_future();
            auto onStarted = [&notifyStart]()
            {
                notifyStart.set_value();
            };
            thread->started.connect(onStarted);
            thread->start();
            started.wait();
        }
        EXPECT_EQ(1, TestThreadLoop::threadCount);
        getMainThreadData.runOnce();
    }
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST_F(Threads, test_parent_detached_thread_deletes_before_quiting)
{
    TestApp app;

    mox::ThreadPromise notify;
    mox::ThreadFuture notifyWait = notify.get_future();

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        auto slot = [&notify]()
        {
            notify.set_value();
        };
        thread->stopped.connect(slot);
        mox::ThreadPromise notifyStart;
        mox::ThreadFuture started = notifyStart.get_future();
        auto onStarted = [&notifyStart]()
        {
            notifyStart.set_value();
        };
        thread->started.connect(onStarted);
        thread->start();
        started.wait();
    }
    EXPECT_EQ(1, TestThreadLoop::threadCount);
    app.runOnce();
    notifyWait.wait();
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST_F(Threads, test_quit_application_from_thread_kills_thread)
{
    TestApp app;
    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        auto onEvQuit = [](auto&)
        {
            mox::Application::instance().quit();
        };
        thread->addEventHandler(evQuit, onEvQuit);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoop>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        app.threadData()->thread()->addIdleTask(onIdle);

        thread->start();
    }

    app.run();
    watchDeath.wait();
}

TEST_F(Threads, test_threads2)
{
    mox::Application getMainThreadData;

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();

    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::getThisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        // Add 2 child objects to thread
        mox::Object::create(thread.get());
        auto c2 = mox::Object::create(thread.get());
        mox::Object::create(c2.get());

        mox::ThreadPromise notifyStart;
        mox::ThreadFuture started = notifyStart.get_future();
        auto onStarted = [&notifyStart]()
        {
            notifyStart.set_value();
        };
        thread->started.connect(onStarted);
        thread->start();
        started.wait();

        auto mainExit = []()
        {
            mox::Application::instance().exit(101);
        };
        thread->stopped.connect(mainExit);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoop>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        getMainThreadData.threadData()->thread()->addIdleTask(onIdle);
    }

    EXPECT_EQ(101, getMainThreadData.run());
    EXPECT_EQ(0, TestThreadLoop::threadCount);
    watchDeath.wait();
}

TEST_F(Threads, test_signal_connected_to_different_thread)
{
    mox::Application app;
    auto newRoot = Quitter::create();
    app.setRootObject(*newRoot);

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoop::create(std::move(notifyDeath));
        auto quitter = app.castRootObject<Quitter>();
        EXPECT_NOT_NULL(thread->stopped.connect(*quitter, &Quitter::quit));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::getThisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        mox::ThreadPromise notifyStart;
        mox::ThreadFuture started = notifyStart.get_future();
        auto onStarted = [&notifyStart]()
        {
            notifyStart.set_value();
        };
        thread->started.connect(onStarted);
        thread->start();
        started.wait();
        EXPECT_EQ(1, TestThreadLoop::threadCount);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoop>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        app.threadData()->thread()->addIdleTask(onIdle);
    }

    EXPECT_EQ(10, app.run());
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}
