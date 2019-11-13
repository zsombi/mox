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
#include <mox/utils/flat_map.hpp>

TEST(FlatMapTests, test_empty_map)
{
    mox::FlatMap<int, int> test;
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(0u, test.size());
    EXPECT_EQ(test.begin(), test.end());
    EXPECT_EQ(test.cbegin(), test.cend());
    EXPECT_EQ(test.rbegin(), test.rend());
    EXPECT_EQ(test.crbegin(), test.crend());
}

TEST(FlatMapTests, test_build_map_from_vector_of_pairs)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {5, 3}, {1, 4}});
    mox::FlatMap<int, int> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(1, test[0].first);
    EXPECT_EQ(4, test[1].first);
    EXPECT_EQ(5, test[2].first);
    EXPECT_EQ(7, test[3].first);
}

TEST(FlatMapTests, test_build_map_from_array)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {5, 3}, {1, 4}});
    mox::FlatMap<int, int> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(1, test[0].first);
    EXPECT_EQ(4, test[1].first);
    EXPECT_EQ(5, test[2].first);
    EXPECT_EQ(7, test[3].first);
}

TEST(FlatMapTests, test_descending_map)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {5, 3}, {1, 4}});
    mox::FlatMap<int, int, std::greater<int>> test(v.cbegin(), v.cend());

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_NE(test.begin(), test.end());
    EXPECT_NE(test.cbegin(), test.cend());
    EXPECT_NE(test.rbegin(), test.rend());
    EXPECT_NE(test.crbegin(), test.crend());

    EXPECT_EQ(7, test[0].first);
    EXPECT_EQ(5, test[1].first);
    EXPECT_EQ(4, test[2].first);
    EXPECT_EQ(1, test[3].first);
}

TEST(FlatMapTests, test_container_initializer)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {5, 3}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    EXPECT_FALSE(test.empty());
    EXPECT_EQ(4u, test.size());
    EXPECT_EQ(1, test[0].first);
    EXPECT_EQ(4, test[1].first);
    EXPECT_EQ(5, test[2].first);
    EXPECT_EQ(7, test[3].first);
}

TEST(FlatMapTests, test_find)
{
    using KeyType = std::pair<int, std::string>;
    std::array<KeyType, 3> a({ std::make_pair(7, "seven"), std::make_pair(1, "one"), std::make_pair(4, "four")});
    mox::FlatMap<int, std::string> test(a);

    EXPECT_EQ(++test.begin(), test.find(4));
    EXPECT_EQ(test.end(), test.find(9));
}

TEST(FlatMapTests, test_contains)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {5, 3}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    EXPECT_TRUE(test.contains(4));
    EXPECT_FALSE(test.contains(3));
}

TEST(FlatMapTests, test_insert)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    EXPECT_EQ(3u, test.size());
    auto pos = test.insert({5, -1});
    EXPECT_NE(test.end(), pos);
    EXPECT_EQ(4u, test.size());
    EXPECT_EQ(1, test[0].first);
    EXPECT_EQ(4, test[1].first);
    EXPECT_EQ(5, test[2].first);
    EXPECT_EQ(7, test[3].first);

    // FIXME: updates the value at pos!
    EXPECT_EQ(test.end(), test.insert({5, 0}));
    EXPECT_EQ(4u, test.size());
}

TEST(FlatMapTests, test_erase)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    EXPECT_EQ(3u, test.size());
    EXPECT_EQ(test.begin() + 1, test.erase(4));
    EXPECT_EQ(2u, test.size());

    // Use the free-function erase, FlatSet erase asserts on non-existent value removal.
    mox::erase(test, std::make_pair(4, 2));
    EXPECT_EQ(2u, test.size());
}

TEST(FlatMapTests, test_forward_iterate)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    auto it = test.begin();
    EXPECT_EQ(it->first, 1); it++;
    EXPECT_EQ(it->first, 4); it++;
    EXPECT_EQ(it->first, 7); it++;
    EXPECT_EQ(it, test.end());
}

TEST(FlatMapTests, test_const_forward_iterate)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    auto it = test.cbegin();
    EXPECT_EQ(it->first, 1); it++;
    EXPECT_EQ(it->first, 4); it++;
    EXPECT_EQ(it->first, 7); it++;
    EXPECT_EQ(it, test.cend());
}

TEST(FlatMapTests, test_backward_iterate)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    auto it = test.rbegin();
    EXPECT_EQ(it->first, 7); it++;
    EXPECT_EQ(it->first, 4); it++;
    EXPECT_EQ(it->first, 1); it++;
    EXPECT_EQ(it, test.rend());
}

TEST(FlatMapTests, test_const_backward_iterate)
{
    std::vector<std::pair<int, int>> v({{7, 1}, {4, 2}, {1, 4}});
    mox::FlatMap<int, int> test(v);

    auto it = test.crbegin();
    EXPECT_EQ(it->first, 7); it++;
    EXPECT_EQ(it->first, 4); it++;
    EXPECT_EQ(it->first, 1); it++;
    EXPECT_EQ(it, test.crend());
}
