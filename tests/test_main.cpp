#include "test_framework.h"
#include <mox/utils/globals.hpp>
#include <gtest/gtest.h>

#include <mox/metadata/metaobject.hpp>

/******************************************************************************
 *
 */
void UnitTest::SetUp()
{
}
void UnitTest::TearDown()
{
}
/******************************************************************************
 *
 */

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    mox::registerMetaType<mox::MetaObject>();
    return RUN_ALL_TESTS();
}
