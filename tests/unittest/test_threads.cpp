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
        auto object = createObject(new Quitter, parent);
        object->addEventHandler(evQuit, std::bind(&Quitter::onEvQuit, object.get(), std::placeholders::_1));
        return object;
    }

    void quit()
    {
        TRACE("Stop the thread");
        threadData()->thread()->exit(10);
    }

protected:
    explicit Quitter() = default;

    void onEvQuit(mox::Event&)
    {
        quit();
    }
};

TEST(Threads, test_thread_basics)
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
    test->addEventHandler(mox::BaseEvent, exiter);

    // exit wait
    mox::ThreadPromise ping;
    auto wait = ping.get_future();
    auto onStopped = [&ping]()
    {
        ping.set_value();
    };
    test->stopped.connect(onStopped);

    // Post a message to the thread to quit the thread
    EXPECT_TRUE(mox::postEvent<mox::Event>(test, mox::BaseEvent));

    wait.wait();
    app.runOnce();
    EXPECT_EQ(mox::ThreadLoop::Status::InactiveOrJoined, mox::ThreadLoop::Status(test->status));
}

TEST(Threads, test_parent_thread_deletes_before_quiting)
{
    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        TestApp getMainThreadData;

        startThreadAndWait(*TestThreadLoopWithDeathNotifier::create(std::move(notifyDeath)));
        EXPECT_EQ(1, TestThreadLoopWithDeathNotifier::threadCount);
        getMainThreadData.runOnce();
    }
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
}

TEST(Threads, test_stopped_signal_received)
{
    TestApp app;

    mox::ThreadPromise notify;
    mox::ThreadFuture notifyWait = notify.get_future();

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoopWithDeathNotifier::create(std::move(notifyDeath));
        auto slot = [&notify]()
        {
            notify.set_value();
        };
        thread->stopped.connect(slot);
        startThreadAndWait(*thread);
    }
    EXPECT_EQ(1, TestThreadLoopWithDeathNotifier::threadCount);
    app.runOnce();
    notifyWait.wait();
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
}

TEST(Threads, test_quit_application_from_thread_kills_thread)
{
    TestApp app;
    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoopWithDeathNotifier::create(std::move(notifyDeath));
        auto onEvQuit = [](auto&)
        {
            mox::Application::instance().quit();
        };
        thread->addEventHandler(evQuit, onEvQuit);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoopWithDeathNotifier>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        app.threadData()->thread()->onIdle(onIdle);

        thread->start();
    }

    app.run();
    watchDeath.wait();
}

TEST(Threads, test_threads)
{
    mox::Application testApp;

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();

    {
        auto thread = TestThreadLoopWithDeathNotifier::create(std::move(notifyDeath));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit.first)
            {
                mox::ThreadData::getThisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        // Add 2 child objects to thread
        mox::Object::create(thread.get());
        auto c2 = mox::Object::create(thread.get());
        mox::Object::create(c2.get());

        startThreadAndWait(*thread);

        auto mainExit = []()
        {
            mox::Application::instance().exit(101);
        };
        thread->stopped.connect(mainExit);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoopWithDeathNotifier>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        testApp.threadData()->thread()->onIdle(onIdle);
    }

    EXPECT_EQ(1, TestThreadLoopWithDeathNotifier::threadCount);
    EXPECT_EQ(101, testApp.run());
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
    watchDeath.wait();
}

TEST(Threads, test_signal_connected_to_different_thread)
{
    mox::Application app;
    auto newRoot = Quitter::create();
    app.setRootObject(*newRoot);

    mox::ThreadPromise notifyDeath;
    mox::ThreadFuture watchDeath = notifyDeath.get_future();
    {
        auto thread = TestThreadLoopWithDeathNotifier::create(std::move(notifyDeath));
        auto quitter = app.castRootObject<Quitter>();
        EXPECT_NOT_NULL(thread->stopped.connect(*quitter, &Quitter::quit));

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit.first)
            {
                mox::ThreadData::getThisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        startThreadAndWait(*thread);
        EXPECT_EQ(1, TestThreadLoopWithDeathNotifier::threadCount);

        auto onIdle = [wthread = std::weak_ptr<TestThreadLoopWithDeathNotifier>(thread)]()
        {
            auto thread = wthread.lock();
            if (thread)
            {
                mox::postEvent<mox::Event>(thread, evQuit);
            }
            return true;
        };
        app.threadData()->thread()->onIdle(onIdle);
    }

    EXPECT_EQ(10, app.run());
    watchDeath.wait();
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
}

TEST(Threads, test_signal_synced_quit_threads_when_app_quits)
{
    mox::Application app;
    mox::ThreadPromise death1;
    mox::ThreadFuture wait1= death1.get_future();
    mox::ThreadPromise death2;
    mox::ThreadFuture wait2= death2.get_future();

    auto t1 = TestThreadLoopWithDeathNotifier::create(std::move(death1));
    auto t2 = TestThreadLoopWithDeathNotifier::create(std::move(death2));
    startThreadAndWait(*t1);
    startThreadAndWait(*t2);
    t1.reset();
    t2.reset();
    EXPECT_EQ(2, TestThreadLoopWithDeathNotifier::threadCount);

    auto root = Quitter::create();
    app.setRootObject(*root);

    // trigger the quit
    mox::postEvent<mox::Event>(root, evQuit);
    app.run();
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
    wait1.wait();
    wait2.wait();
}

TEST(Threads, test_signal_synced_quit_app_when_thread_quits)
{
    mox::Application app;
    mox::ThreadPromise death1;
    mox::ThreadFuture wait1= death1.get_future();
    mox::ThreadPromise death2;
    mox::ThreadFuture wait2= death2.get_future();

    auto t1 = TestThreadLoopWithDeathNotifier::create(std::move(death1));
    auto t2 = TestThreadLoopWithDeathNotifier::create(std::move(death2));
    startThreadAndWait(*t1);
    startThreadAndWait(*t2);
    t1->stopped.connect(app, &mox::Application::quit);
    t2->stopped.connect(app, &mox::Application::quit);
    t1.reset();
    t2.reset();
    EXPECT_EQ(2, TestThreadLoopWithDeathNotifier::threadCount);

    auto quitter = Quitter::create(t1.get());

    auto idle = [target = std::weak_ptr<Quitter>(quitter)]()
    {
        auto locked = target.lock();
        EXPECT_NOT_NULL(locked);
        mox::postEvent<mox::Event>(locked, evQuit);
        return true;
    };
    app.threadData()->thread()->onIdle(idle);
    app.run();
    EXPECT_EQ(0, TestThreadLoopWithDeathNotifier::threadCount);
    wait1.wait();
    wait2.wait();
}

TEST(Threads, test_nested_threads_cleanup)
{
    mox::Application app;
    app.setRootObject(*Quitter::create());
    auto t1 = TestThreadLoop::create();

    auto onStarted = []()
    {
        auto t2 = TestThreadLoop::create();
        startThreadAndWait(*t2);
        mox::Connection::getActiveConnection()->disconnect();
    };
    t1->started.connect(onStarted);
    startThreadAndWait(*t1);
    EXPECT_EQ(2, TestThreadLoop::threadCount);

    mox::postEvent<mox::Event>(app.getRootObject(), evQuit);
    app.run();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}

TEST(Threads, test_more_threads)
{
    mox::Application app;
    app.setRootObject(*Quitter::create());

    for (int i = 0; i < 5; i++)
    {
        auto thread = TestThreadLoop::create();

        auto onStarted = []()
        {
            auto t2 = TestThreadLoop::create();
            startThreadAndWait(*t2);
            mox::Connection::getActiveConnection()->disconnect();
        };
        thread->started.connect(onStarted);
        startThreadAndWait(*thread);
        EXPECT_EQ((i + 1)* 2, TestThreadLoop::threadCount);
    }

    mox::postEvent<mox::Event>(app.getRootObject(), evQuit);
    app.run();
    EXPECT_EQ(0, TestThreadLoop::threadCount);
}
