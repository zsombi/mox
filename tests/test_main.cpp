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
}
void UnitTest::TearDown()
{
    ::testing::Test::TearDown();
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
