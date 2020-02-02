#include "test_framework.h"
#include <mox/utils/globals.hpp>
#include <gtest/gtest.h>

#include <mox/mox_module.hpp>

/******************************************************************************
 *
 */
void UnitTest::SetUp()
{
    ::testing::Test::SetUp();
    mox::registerMetaType<TestApp>();
    mox::registerMetaType<TestThreadLoop>();
    mox::registerMetaType<TestThreadLoop*>();
}
void UnitTest::TearDown()
{
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
