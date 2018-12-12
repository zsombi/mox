#include "test_framework.h"
#include <mox/utils/globals.hpp>
#include <gtest/gtest.h>

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
    return RUN_ALL_TESTS();
}
