#include "test_framework.h"
#include <gtest/gtest.h>

#include "../src/include/private/process_p.hpp"

/******************************************************************************
 *
 */
UnitTest::LogData::LogData(mox::LogCategory* category, mox::LogType type, std::string message, int expectedCount)
    : category(category)
    , type(type)
    , message(message)
    , expectedCount(expectedCount)
{
}

bool UnitTest::LogData::operator==(const LogData& other)
{
    return (category == other.category) &&
            (type == other.type) &&
            (message == other.message);
}

UnitTest::TestLogger::TestLogger(UnitTest& fixup)
    : fixup(fixup)
{
}
UnitTest::TestLogger::~TestLogger()
{
}

bool UnitTest::TestLogger::log(mox::LogCategory& category, mox::LogType type, std::string_view heading, const std::string& text)
{
    LogData logData(&category, type, text, true);
    auto find = [&logData] (auto& data)
    {
        return logData == data;
    };
    auto it = mox::find_if(fixup.trackedLogs, find);
    if (it != fixup.trackedLogs.end())
    {
        if (it->expectedCount > 0)
        {
            it->occurence++;
        }
        else
        {
            it->occurence--;
        }
        return false;
    }

    return ScreenLogger::log(category, type, heading, text);
}

void UnitTest::testLogs()
{
    for (auto& log : trackedLogs)
    {
        EXPECT_EQ(log.expectedCount, log.occurence);
    }
}

/******************************************************************************
 *
 */
void UnitTest::expectLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count)
{
    trackedLogs.emplace_back(LogData(category, type, ' ' + std::string(message), count));
}

void UnitTest::expectNoLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count)
{
    trackedLogs.emplace_back(LogData(category, type, ' ' + std::string(message), -count));
}

void UnitTest::SetUp()
{
    ::testing::Test::SetUp();

    mox::Logger::setLogger(std::make_unique<TestLogger>(*this));
}
void UnitTest::TearDown()
{
    testLogs();
    trackedLogs.clear();
    mox::Logger::setLogger(std::make_unique<mox::ScreenLogger>());

    ::testing::Test::TearDown();
}

/******************************************************************************
 * TestThreadLoop
 */
std::shared_ptr<TestThreadLoop> TestThreadLoop::create(mox::ThreadPromise&& notifier)
{
    return make_thread(new TestThreadLoop(std::forward<mox::ThreadPromise>(notifier)));
}

TestThreadLoop::~TestThreadLoop()
{
    m_deathNotifier.set_value();
}

TestThreadLoop::TestThreadLoop(mox::ThreadPromise&& notifier)
    : m_deathNotifier(std::move(notifier))
{
}

void TestThreadLoop::initialize()
{
    ThreadLoop::initialize();
    started.connect(*this, &TestThreadLoop::onStarted);
    stopped.connect(*this, &TestThreadLoop::onStopped);
}

void TestThreadLoop::onStarted()
{
    ++threadCount;
}
void TestThreadLoop::onStopped()
{
    --threadCount;
}

/******************************************************************************
 *
 */

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
