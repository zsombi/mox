#include "test_framework.h"
#include <gtest/gtest.h>

#include <mox/mox_module.hpp>
#include "../src/include/private/process_p.hpp"

/******************************************************************************
 *
 */
#if defined(MOX_ENABLE_LOGS)

UnitTest::LogData::LogData(mox::LogCategory* category, mox::LogType type, std::string_view message)
    : category(category)
    , type(type)
    , message(message)
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
    LogData logData(&category, type, text);
    auto find = [&logData] (auto& data)
    {
        return logData == data;
    };
    auto it = std::find_if(fixup.expectedLogs.begin(), fixup.expectedLogs.end(), find);
    if (it != fixup.expectedLogs.end())
    {
        fixup.expectedLogs.erase(it);
        return false;
    }
    else
    {
        return ScreenLogger::log(category, type, heading, text);
    }
}

void UnitTest::testLogs()
{
    EXPECT_EQ(0u, expectedLogs.size());
}

/******************************************************************************
 *
 */
void UnitTest::expectLog(mox::LogCategory* category, mox::LogType type, std::string_view message, size_t count)
{
    while (count--)
    {
        expectedLogs.emplace_back(LogData(category, type, message));
    }
}
#endif

void UnitTest::SetUp()
{
    ::testing::Test::SetUp();

#if defined(MOX_ENABLE_LOGS)
    mox::Logger::setLogger(std::make_unique<TestLogger>(*this));
#endif
    mox::registerMetaType<TestApp>();
    mox::registerMetaClass<TestThreadLoop>();
}
void UnitTest::TearDown()
{
#if defined(MOX_ENABLE_LOGS)
    testLogs();
    expectedLogs.clear();
    mox::Logger::setLogger(std::make_unique<mox::ScreenLogger>());
#endif

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
    mox::MoxModule module;
    module.registerModule();
    mox::registerMetaClass<TestApp>();

    return RUN_ALL_TESTS();
}
