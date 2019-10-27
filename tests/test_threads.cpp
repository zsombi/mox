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

#include <mox/module/thread_loop.hpp>
#include <mox/object.hpp>
#include "test_framework.h"

class TestThread;
using TestThreadSharedPtr = std::shared_ptr<TestThread>;

class TestThread : public mox::ThreadLoop
{
public:
    static inline int threadCount = 0;
    static TestThreadSharedPtr create(Object* parent = nullptr)
    {
        return createObject(new TestThread, parent);
    }

protected:
    void run() override
    {
        ++threadCount;
        ThreadLoop::run();
        --threadCount;
    }
};

TEST(Threads, test_thread_basics)
{
    TestModule mainThread;

    auto test = mox::ThreadLoop::create();
    test->start();

    EXPECT_NE(test->threadData(), mox::ThreadData::thisThreadData());

    // event handler to stop the thread
    auto exiter = [](mox::Event&)
    {
        mox::ThreadLoop::thisThread()->exit(0);
    };
    test->addEventHandler(mox::EventType::Base, exiter);

    // Post a message to the thread to quit the thread
    EXPECT_TRUE(mox::postEvent<mox::Event>(mox::EventType::Base, test));

    test->handler().join();
    EXPECT_EQ(mox::ThreadLoop::Status::Stopped, test->getStatus());
}

TEST(Threads, test_parented_thread_deletes_before_quiting)
{
    TestModule mainThread;
    auto root = mox::Object::create();

    TestThread::create(root.get())->start();
    EXPECT_EQ(1, TestThread::threadCount);
    root.reset();
    EXPECT_EQ(0, TestThread::threadCount);
}

TEST(Threads, test_parented_detached_thread_deletes_before_quiting)
{
    TestModule mainThread;
    auto root = mox::Object::create();
    TestThread::create(root.get())->start().handler().detach();
    EXPECT_EQ(1, TestThread::threadCount);

    root.reset();
    EXPECT_EQ(0, TestThread::threadCount);
}

static mox::EventType evQuit = mox::Event::registerNewType();
class Quitter : public mox::Object
{
public:

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, Quitter, mox::Object>
    {
        Method quit{*this, &Quitter::quit, "quit"};
    };

    static std::shared_ptr<Quitter> create(mox::Object* parent = nullptr)
    {
        return createObject(new Quitter, parent);
    }

    void quit()
    {
        TRACE("Stop main thread")
        threadData()->eventLoop()->exit(10);
    }
};

TEST(Threads, test_signal_connected_to_different_thread)
{
    mox::registerMetaClass<Quitter>();

    TestModule mainThread;
    mox::EventLoop loop;
    auto quitter = Quitter::create();

    mox::ThreadLoopWeakPtr threadCheck;

    {
        auto thread = mox::ThreadLoop::create();
        thread->stopped.connect(*quitter, &Quitter::quit);
        threadCheck = thread;

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::thisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        thread->start();

        mox::postEvent<mox::Event>(evQuit, thread);
    }

    EXPECT_EQ(10, loop.processEvents());
    EXPECT_TRUE(threadCheck.expired());
}

TEST(Threads, test_signal_connected_to_metamethod_in_different_thread)
{
    mox::registerMetaClass<Quitter>();

    TestModule mainThread;
    mox::EventLoop loop;
    auto quitter = Quitter::create();

    mox::ThreadLoopWeakPtr threadCheck;

    {
        auto thread = mox::ThreadLoop::create();
        thread->stopped.connect(*quitter, "quit");
        threadCheck = thread;

        auto quitEventHandler = [](mox::Event& event)
        {
            if (event.type() == evQuit)
            {
                mox::ThreadData::thisThreadData()->thread()->exit(0);
            }
        };
        thread->addEventHandler(evQuit, quitEventHandler);

        thread->start();

        mox::postEvent<mox::Event>(evQuit, thread);
    }

    EXPECT_EQ(10, loop.processEvents());
    EXPECT_TRUE(threadCheck.expired());
}
