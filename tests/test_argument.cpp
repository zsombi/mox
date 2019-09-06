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

#include <mox/metadata/argument.hpp>
#include <mox/utils/function_traits.hpp>

TEST(Argument, test_argument_init)
{
    mox::Argument i(10);
    int v = i;
    EXPECT_EQ(10, v);
}

TEST(Argument, test_assign_value)
{
    mox::Argument v;
    EXPECT_FALSE(v.isValid());

    v = 10;
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(mox::Metatype::Int, v.metaType());
    EXPECT_EQ(10, v);

    v = std::string_view("apple");
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(mox::Metatype::Literal, v.metaType());
    EXPECT_EQ(std::string_view("apple"), v);

    v = 23.4;
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(mox::Metatype::Double, v.metaType());
    EXPECT_EQ(23.4, v);

    v = 123.45f;
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(mox::Metatype::Float, v.metaType());
    EXPECT_EQ(123.45f, v);
}

TEST(Argument, test_base_type_convert)
{
    mox::Argument v(10);
    bool b = v;
    EXPECT_TRUE(b);

    char c = v;
    EXPECT_EQ('\n', c);

    byte bv = v;
    EXPECT_EQ(byte(10), bv);

    short s = v;
    EXPECT_EQ(10, s);

    unsigned short w = v;
    EXPECT_EQ(10, w);

    int i = v;
    EXPECT_EQ(10, i);

    unsigned int ui = v;
    EXPECT_EQ(10, ui);

    long l = v;
    EXPECT_EQ(10l, l);

    unsigned long ul = v;
    EXPECT_EQ(10u, ul);

    // change v type
    v = 101l;
    EXPECT_EQ(mox::Metatype::Long, v.metaType());

    b = v;
    EXPECT_TRUE(b);

    c = v;
    EXPECT_EQ('e', c);

    const char cc = v;
    EXPECT_EQ('e', cc);
}

TEST(Argument, test_string_casts)
{
    mox::Argument v(true);

    std::string ss = v;
    EXPECT_EQ("true", ss);

    v = 123;
    int i = v;
    EXPECT_EQ(123, i);

    v = std::string("true");
    bool b = v;
    EXPECT_TRUE(b);

    // conversion fails
    EXPECT_THROW(i = v, mox::bad_conversion);

    v = std::string("10");
    i = v;
    EXPECT_EQ(10, i);

    v = std::string("15.11");
    i = v;
    EXPECT_EQ(15, i);
}

TEST(Argument, test_cstring_to_number)
{
    mox::Argument v;
    v = std::string_view("101");

    int i = v;
    EXPECT_EQ(i, 101);
}

TEST(Argument, test_string_literal)
{
    mox::Argument v;
    v = std::string_view("true");
    bool b = v;
    EXPECT_TRUE(b);

    std::string s = v;
    EXPECT_EQ(s, std::string("true"));
}

TEST(Argument, test_hex_string_to_number)
{
    mox::Argument v(std::string("0xFF"));
    int i = v;
    EXPECT_EQ(255, i);

    double d = v;
    EXPECT_EQ(255.0f, d);
}

TEST(Argument, test_hex_literal_to_number)
{
    mox::Argument v(std::string_view("0xFF"));
    int i = v;
    EXPECT_EQ(255, i);

    double d = v;
    EXPECT_EQ(255.0f, d);
}

TEST(Argument, test_bad_string_to_number_throws)
{
    mox::Argument v(std::string("fadabec"));
    int i = 0;
    EXPECT_THROW(i = v, mox::bad_conversion);
}

TEST(Argument, test_bad_literal_to_number_throws)
{
    mox::Argument v(std::string_view("fadabec"));
    int i = 0;
    EXPECT_THROW(i = v, mox::bad_conversion);
}

TEST(Argument, test_argument_descriptor_operators_lsv_rsv)
{
    mox::ArgumentDescriptor a1(mox::Metatype::String, false, false);
    mox::ArgumentDescriptor a2(mox::Metatype::Literal, false, false);

    EXPECT_FALSE(a2.invocableWith(a1));
    EXPECT_TRUE(a1.invocableWith(a2));
}
