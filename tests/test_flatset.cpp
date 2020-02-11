/*
 * Copyright (C) 2017-2019 bitWelder
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

#include "test_framework.h"
#include <mox/utils/containers/flat_set.hpp>

TEST(FlatSetTests, test_empty_set)
{
    mox::FlatSet<int> test;
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(0u, test.size());
    EXPECT_EQ(test.begin(), test.end());
    EXPECT_EQ(test.cbegin(), test.cend());
    EXPECT_EQ(test.rbegin(), test.rend());
    EXPECT_EQ(test.crbegin(), test.crend());
}

TEST(FlatSetTests, test_build_set_from_vector)
{
    std::vector<int> v({7, 4, 5, 1});
    mox::FlatSet<int> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(1, test[0]);
    EXPECT_EQ(4, test[1]);
    EXPECT_EQ(5, test[2]);
    EXPECT_EQ(7, test[3]);
}

TEST(FlatSetTests, test_build_set_from_array)
{
    std::array<int, 4> v({7, 4, 5, 1});
    mox::FlatSet<int> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(1, test[0]);
    EXPECT_EQ(4, test[1]);
    EXPECT_EQ(5, test[2]);
    EXPECT_EQ(7, test[3]);
}

TEST(FlatSetTests, test_descending_set)
{
    std::array<int, 4> v({7, 4, 5, 1});
    mox::FlatSet<int, std::greater<int>> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(7, test[0]);
    EXPECT_EQ(5, test[1]);
    EXPECT_EQ(4, test[2]);
    EXPECT_EQ(1, test[3]);
}

TEST(FlatSetTests, test_container_initializer)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(3u, test.size());
    EXPECT_EQ(1, test[0]);
    EXPECT_EQ(4, test[1]);
    EXPECT_EQ(7, test[2]);
}

TEST(FlatSetTests, test_find)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    EXPECT_EQ(++test.begin(), test.find(4));
    EXPECT_EQ(test.end(), test.find(5));
}

TEST(FlatSetTests, test_contains)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    EXPECT_TRUE(test.contains(4));
    EXPECT_FALSE(test.contains(5));
}

TEST(FlatSetTests, test_insert)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    EXPECT_EQ(3u, test.size());
    EXPECT_NE(test.end(), test.insert(5));
    EXPECT_EQ(4u, test.size());
    EXPECT_EQ(1, test[0]);
    EXPECT_EQ(4, test[1]);
    EXPECT_EQ(5, test[2]);
    EXPECT_EQ(7, test[3]);

    EXPECT_EQ(test.end(), test.insert(5));
    EXPECT_EQ(4u, test.size());
}

TEST(FlatSetTests, test_erase)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    EXPECT_EQ(3u, test.size());
    EXPECT_EQ(test.begin() + 1, test.erase(4));
    EXPECT_EQ(2u, test.size());

    // Use the free-function erase, FlatSet erase asserts on non-existent value removal.
    mox::erase(test, 4);
    EXPECT_EQ(2u, test.size());
}

TEST(FlatSetTests, test_forward_iterate)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    auto it = test.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 7);
    EXPECT_EQ(it, test.end());
}

TEST(FlatSetTests, test_const_forward_iterate)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    auto it = test.cbegin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 7);
    EXPECT_EQ(it, test.cend());
}

TEST(FlatSetTests, test_backward_iterate)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    auto it = test.rbegin();
    EXPECT_EQ(*it++, 7);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(it, test.rend());
}

TEST(FlatSetTests, test_const_backward_iterate)
{
    std::array<int, 3> v({7, 4, 1});
    mox::FlatSet<int> test(v);

    auto it = test.crbegin();
    EXPECT_EQ(*it++, 7);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(it, test.crend());
}
