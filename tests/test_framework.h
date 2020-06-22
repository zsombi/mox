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

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <gtest/gtest.h>
#include <mox/config/deftypes.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/process/application.hpp>
#include <mox/core/process/thread_loop.hpp>
#include <mox/config/error.hpp>
#include <mox/utils/log/logger.hpp>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class UnitTest : public ::testing::Test
{
    struct LogData
    {
        mox::LogCategory* category;
        mox::LogType type;
        std::string message;
        const int expectedCount = 0;
        int occurence = 0;
        LogData(mox::LogCategory* category, mox::LogType type, std::string message, int expectedCount);

        bool operator==(const LogData& other);
    };
    using LogContainer = std::vector<LogData>;

    LogContainer trackedLogs;

    class TestLogger : public mox::ScreenLogger
    {
        UnitTest& fixup;
    public:
        TestLogger(UnitTest& fixup);
        ~TestLogger() override;
        bool log(mox::LogCategory& category, mox::LogType type, std::string_view heading, const std::string &text) override;
    };

    void testLogs();

protected:
    template <mox::LogType types>
    struct ScopeLogType
    {
        ScopeLogType(std::string_view category)
        {
            _cat = mox::Logger::findCategory(category);
            if (!_cat)
            {
                auto id = mox::Logger::addCategory(mox::LogCategory(category));
                _cat = &mox::Logger::getCategory(id);
            }
            _bak = _cat->getTypes();
            _cat->setTypes(types);
        }
        ~ScopeLogType()
        {
            _cat->setTypes(_bak);
        }
    private:
        mox::LogCategory* _cat = nullptr;
        mox::LogType _bak;
    };

    void expectLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count = 1);
    void expectNoLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count = 1);

    void SetUp() override;
    void TearDown() override;
};

class TestCoreApp
{
    static inline TestCoreApp* m_instance = nullptr;
public:
    explicit TestCoreApp();
    ~TestCoreApp();
    static TestCoreApp* instance();
    void exit();
    void run();
    void runOnce();
    void runOnce(mox::IdleFunction exitTask);
    void addIdleTask(mox::IdleFunction idle);

    static void onExit()
    {
        instance()->exit();
    }

    class Private;
    std::unique_ptr<Private> d;
};

class QuitHandler : public mox::Object
{
public:
    static std::shared_ptr<QuitHandler> create(mox::EventType typeToQuit, mox::Object* parent = nullptr);

protected:
    explicit QuitHandler() = default;

    void onEvent(mox::Event& event);
};

class TestApp : public mox::Application
{
    static inline mox::EventType testAppQuit = mox::Event::registerNewType();

public:
    explicit TestApp();

    int runOnce();
};

class TestThreadLoop : public mox::ThreadLoop
{
public:
    static inline std::atomic_int threadCount = 0;

    static std::shared_ptr<TestThreadLoop> create();

protected:
    explicit TestThreadLoop() = default;

    void initialize() override;

    void onStarted();
    void onStopped();
};

class TestThreadLoopWithDeathNotifier : public TestThreadLoop
{
    mox::ThreadPromise m_deathNotifier;

public:
    static std::shared_ptr<TestThreadLoopWithDeathNotifier> create(mox::ThreadPromise&& notifier);

    ~TestThreadLoopWithDeathNotifier() override;

protected:
    explicit TestThreadLoopWithDeathNotifier(mox::ThreadPromise&& notifier);
};

void startThreadAndWait(mox::ThreadLoop& thread);

#define SLEEP(msec)             std::this_thread::sleep_for(std::chrono::milliseconds(msec))
#define EXPECT_NULL(ptr)        EXPECT_EQ(nullptr, ptr)
#define EXPECT_NOT_NULL(ptr)    EXPECT_NE(nullptr, ptr)
#define EXPECT_NOT_EQ(A, B)     EXPECT_NE(A, B)

#define EXPECT_TRACE(c, message)    expectLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Debug, message, 1)
#define EXPECT_WARNING(c, message)  expectLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Warning, message, 1)
#define EXPECT_INFO(c, message)     expectLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Info, message, 1)

#define EXPECT_NO_TRACE(c, message)    expectNoLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Debug, message, 1)
#define EXPECT_NO_WARNING(c, message)  expectNoLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Warning, message, 1)
#define EXPECT_NO_INFO(c, message)     expectNoLog(mox::Logger::findCategory(CATEGORY(c)), mox::LogType::Info, message, 1)

#endif // TEST_FRAMEWORK_H
