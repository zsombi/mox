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
#include <mox/metadata/variant.hpp>
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metaobject.hpp>

class TestObject : public mox::MetaObject
{
public:
    METACLASS(TestObject, MetaObject)
    {};
    explicit TestObject()
    {
    }
};

TEST(Variants, test_baseTypes)
{
    mox::Variant v;

    EXPECT_FALSE(v);
}

TEST(Variants, test_value)
{
    mox::Variant v(12);
    EXPECT_EQ(12, v);
    EXPECT_THROW(v.value<float>(), std::bad_variant_access);
}

TEST(Variants, test_equality_operators)
{
    mox::Variant v1, v2;
    EXPECT_TRUE(v1 == v2);

    v1 = 10;
    EXPECT_TRUE(10 == v1);
    EXPECT_TRUE(v1 == 10);
}

TEST(Variants, test_convert)
{
    mox::Variant var;
    var = 123;

    mox::Variant var2(mox::variant_cast<float>(var));
    EXPECT_TRUE(var2);
    EXPECT_EQ(123.0f, var2);

    EXPECT_EQ(123.0f, mox::variant_cast<float>(var));
}

TEST(Variants, test_convert_string)
{
    mox::Variant var(std::string("True"));
    EXPECT_EQ(mox::Metatype::String, var.type());
    EXPECT_TRUE(mox::variant_cast<bool>(var));

    var = false;
    EXPECT_EQ(mox::Metatype::Bool, var.type());
    EXPECT_EQ("false", mox::variant_cast<std::string>(var));
}

TEST(Variants, test_metaobject)
{
    TestObject obj;
    void* p1 = &obj;
    mox::Variant var;

    mox::registerMetaType<TestObject>();

    var = p1;
    EXPECT_EQ(mox::Metatype::VoidPtr, var.type());
    EXPECT_EQ(p1, var);

    var = &obj;
    EXPECT_NE(mox::Metatype::MetaObject, var.type());
    EXPECT_EQ(TestObject::StaticMetaClass::get()->metaType(), var.type());
    EXPECT_EQ(&obj, var);
}
