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

#include <mox/meta/core/variant.hpp>
#include <mox/utils/function_traits.hpp>

TEST(Variant, test_argument_init)
{
    mox::Variant i(10);
    int v = i;
    EXPECT_EQ(10, v);
}

TEST(Variant, test_assign_value)
{
    mox::Variant v;
    EXPECT_FALSE(v.isValid());

    v = 10;
    EXPECT_TRUE(v.isValid());
    EXPECT_EQ(mox::Metatype::Int32, v.metaType());
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

TEST(Variant, test_base_type_convert)
{
    mox::Variant v(10);
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

    // change v type
    v = int64_t(101);
    EXPECT_EQ(mox::Metatype::Int64, v.metaType());

    b = v;
    EXPECT_TRUE(b);

    c = v;
    EXPECT_EQ('e', c);

    const char cc = v;
    EXPECT_EQ('e', cc);
}

TEST(Variant, test_string_casts)
{
    mox::Variant v(true);

    std::string ss = v;
    EXPECT_EQ("true", ss);

    v = 123;
    int i = v;
    EXPECT_EQ(123, i);

    v = std::string("true");
    bool b = v;
    EXPECT_TRUE(b);

    // conversion fails
    EXPECT_THROW(i = v, mox::Exception);

    v = std::string("10");
    i = v;
    EXPECT_EQ(10, i);

    v = std::string("15.11");
    i = v;
    EXPECT_EQ(15, i);
}

TEST(Variant, test_cstring_to_number)
{
    mox::Variant v;
    v = std::string_view("101");

    int i = v;
    EXPECT_EQ(i, 101);
}

TEST(Variant, test_string_literal)
{
    mox::Variant v;
    v = std::string_view("true");
    bool b = v;
    EXPECT_TRUE(b);

    std::string s = v;
    EXPECT_EQ(s, std::string("true"));
}

TEST(Variant, test_hex_string_to_number)
{
    mox::Variant v(std::string("0xFF"));
    int i = v;
    EXPECT_EQ(255, i);
}

TEST(Variant, test_hex_literal_to_number)
{
    mox::Variant v(std::string_view("0xFF"));
    int i = v;
    EXPECT_EQ(255, i);
}

TEST(Variant, test_bad_string_to_number_throws)
{
    mox::Variant v(std::string("fadabec"));
    int i = 0;
    EXPECT_THROW(i = v, mox::Exception);
    UNUSED(i);
}

TEST(Variant, test_bad_literal_to_number_throws)
{
    mox::Variant v(std::string_view("fadabec"));
    int i = 0;
    EXPECT_THROW(i = v, mox::Exception);
    UNUSED(i);
}

TEST(Variant, test_argument_descriptor_operators_lsv_rsv)
{
    mox::VariantDescriptor a1(mox::Metatype::String, false, false);
    mox::VariantDescriptor a2(mox::Metatype::Literal, false, false);

    EXPECT_FALSE(a2.invocableWith(a1));
    EXPECT_TRUE(a1.invocableWith(a2));
}

TEST(Variant, test_convert_pointer_to_intptr)
{
    int value = 101;
    int* pvalue = &value;
    mox::Variant var(pvalue);

    EXPECT_EQ(mox::Metatype::Int32Ptr, var.descriptor().getType());

    intptr_t ipvalue = reinterpret_cast<intptr_t>(pvalue);
    intptr_t iptr = var;
    EXPECT_EQ(iptr, ipvalue);
}
