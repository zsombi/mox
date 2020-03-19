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
#include <mox/event_handling/run_loop.hpp>
#include <mox/metatype.core/metadata.hpp>
#include <mox/metatype.core/metatype_descriptor.hpp>
#include <mox/module/application.hpp>
#include <mox/module/thread_loop.hpp>
#include <mox/config/error.hpp>
#include <mox/utils/log/logger.hpp>

class UnitTest : public ::testing::Test
{
#if defined(MOX_ENABLE_LOGS)
    class TestLogger : public mox::ScreenLogger
    {
        static TestLogger* logger;
    public:
        TestLogger();
        ~TestLogger() override;
        bool log(mox::LogCategory& category, mox::LogType type, const std::string &text) override;

        static TestLogger* get()
        {
            return logger;
        }
        void testLogs();

        struct LogData
        {
            mox::LogCategory* category;
            mox::LogType type;
            std::string message;
            LogData(mox::LogCategory* category, mox::LogType type, std::string_view message);

            bool operator==(const LogData& other);
        };
        using LogContainer = std::vector<LogData>;

        LogContainer expectedLogs;
    };
#endif

protected:
#if defined(MOX_ENABLE_LOGS)
    void expectLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count = 1);
#endif
    void SetUp() override;
    void TearDown() override;
};

template <typename T>
class UpdatingPropertyData : public mox::PropertyData<T>
{
public:
    UpdatingPropertyData(T value)
        : mox::PropertyData<T>(value)
    {}

    void update(T value)
    {
        mox::PropertyData<T>::update(mox::Variant(value));
    }
};

class TestApp : public mox::Application
{
public:
    MetaInfo(TestApp, mox::Application)
    {
    };

    explicit TestApp() = default;
    int runOnce()
    {
        auto idleTask = []()
        {
            Application::instance().quit();
            return true;
        };
        addIdleTask(std::move(idleTask));
        return run();
    }
};

using Notifier = std::promise<void>;
using Watcher = std::future<void>;

class TestThreadLoop : public mox::ThreadLoop
{
    Notifier m_deathNotifier;
public:
    static inline std::atomic_int threadCount = 0;

    static std::shared_ptr<TestThreadLoop> create(Notifier&& notifier, Object* parent = nullptr);

    ~TestThreadLoop() override;

protected:
    explicit TestThreadLoop(Notifier&& notifier);

    void init();

    void onStarted();
    void onStopped();
};

#define SLEEP(msec)             std::this_thread::sleep_for(std::chrono::milliseconds(msec))
#define EXPECT_NULL(ptr)        EXPECT_EQ(nullptr, ptr)
#define EXPECT_NOT_NULL(ptr)    EXPECT_NE(nullptr, ptr)
#define EXPECT_NOT_EQ(A, B)     EXPECT_NE(A, B)

#if defined(MOX_ENABLE_LOGS)

#define EXPECT_TRACE(c, message)    expectLog(logCategoryRegistrar_##c.category(), mox::LogType::Debug, message, 1)
#define EXPECT_WARNING(c, message)  expectLog(logCategoryRegistrar_##c.category(), mox::LogType::Warning, message, 1)
#define EXPECT_INFO(c, message)     expectLog(logCategoryRegistrar_##c.category(), mox::LogType::Info, message, 1)

#else

#define EXPECT_TRACE(c, message)
#define EXPECT_WARNING(c, message)
#define EXPECT_INFO(c, message)

#endif

#endif // TEST_FRAMEWORK_H
