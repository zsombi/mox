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
#include <unordered_map>
#include <mox/metadata/metatype.hpp>
#include <mox/metadata/metatype_descriptor.hpp>

class UnitTest : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
};

template <typename Type>
mox::Metatype registerTestType()
{
    typedef typename std::remove_pointer<typename std::remove_reference<Type>::type>::type NakedType;
    const mox::MetatypeDescriptor* descriptor = mox::registrar::findMetatypeDescriptor(typeid(NakedType));
    if (!descriptor)
    {
        return mox::registerMetaType<Type>();
    }
    return descriptor->id();
}

#define SLEEP(msec) std::this_thread::sleep_for(std::chrono::milliseconds(msec))
#define EXPECT_NULL(ptr)        EXPECT_TRUE(ptr == nullptr)
#define EXPECT_NOT_NULL(ptr)    EXPECT_TRUE(ptr != nullptr)

#endif // TEST_FRAMEWORK_H
