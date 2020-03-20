#include "test_framework.h"
#include <gtest/gtest.h>

#include <mox/mox_module.hpp>

/******************************************************************************
 *
 */
#if defined(MOX_ENABLE_LOGS)
UnitTest::TestLogger* UnitTest::TestLogger::logger = nullptr;

UnitTest::TestLogger::LogData::LogData(mox::LogCategory* category, mox::LogType type, std::string_view message)
    : category(category)
    , type(type)
    , message(message)
{
}

bool UnitTest::TestLogger::LogData::operator==(const LogData& other)
{
    return (category == other.category) &&
            (type == other.type) &&
            (message == other.message);
}

UnitTest::TestLogger::TestLogger()
{
    logger = this;
}
UnitTest::TestLogger::~TestLogger()
{
    logger = nullptr;
}

bool UnitTest::TestLogger::log(mox::LogCategory& category, mox::LogType type, const std::string& text)
{
    LogData logData(&category, type, text);
    auto find = [&logData] (auto& data)
    {
        return logData == data;
    };
    auto it = std::find_if(expectedLogs.begin(), expectedLogs.end(), find);
    if (it != expectedLogs.end())
    {
        expectedLogs.erase(it);
        return false;
    }
    else
    {
        return ScreenLogger::log(category, type, text);
    }
}

void UnitTest::TestLogger::testLogs()
{
    EXPECT_EQ(0u, expectedLogs.size());
}

/******************************************************************************
 *
 */
void UnitTest::expectLog(mox::LogCategory *category, mox::LogType type, std::string_view message, size_t count)
{
    while (count--)
    {
        TestLogger::get()->expectedLogs.emplace_back(TestLogger::LogData(category, type, message));
    }
}
#endif

void UnitTest::SetUp()
{
    ::testing::Test::SetUp();

#if defined(MOX_ENABLE_LOGS)
    mox::Logger::setLogger(std::make_unique<TestLogger>());
#endif
    mox::registerMetaType<TestApp>();
    mox::registerMetaType<TestThreadLoop>();
    mox::registerMetaType<TestThreadLoop*>();
}
void UnitTest::TearDown()
{
#if defined(MOX_ENABLE_LOGS)
    TestLogger::get()->testLogs();
    mox::Logger::setLogger(std::make_unique<mox::ScreenLogger>());
#endif

    ::testing::Test::TearDown();
}

/******************************************************************************
 * TestThreadLoop
 */
std::shared_ptr<TestThreadLoop> TestThreadLoop::create(Notifier&& notifier, Object* parent)
{
    auto thread = createObject(new TestThreadLoop(std::forward<Notifier>(notifier)), parent);

    thread->init();

    return thread;
}

TestThreadLoop::~TestThreadLoop()
{
    m_deathNotifier.set_value();
}

TestThreadLoop::TestThreadLoop(Notifier&& notifier)
{
    m_deathNotifier.swap(notifier);
}

void TestThreadLoop::init()
{
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
