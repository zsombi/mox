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
#include <mox/metainfo/metaclass.hpp>
#include <mox/metainfo/metaobject.hpp>
#include <mox/meta/core/callable.hpp>

using namespace mox;

class TestMixin
{
public:
    bool invoked = false;

    virtual ~TestMixin() = default;

    void testFunc1()
    {
        invoked = true;
    }

    int testFunc2()
    {
        return 1234321;
    }

    static int staticFunc(int value)
    {
        return value;
    }

    MetaInfo(TestMixin)
    {
        static inline MetaMethod<TestMixin> testFunc1{&BaseType::testFunc1, "testFunc1"};
        static inline MetaMethod<TestMixin> testFunc2{&BaseType::testFunc2, "testFunc2"};
        static inline MetaMethod<TestMixin> staticFunc{&BaseType::staticFunc, "staticFunc"};
        static MetaMethod<TestMixin> lambda;
    };
};
TestMixin::StaticMetaClass::MetaMethod<TestMixin> TestMixin::StaticMetaClass::lambda([](TestMixin* instance) { instance->invoked = true; }, "lambda");

class TestSecond
{
public:

    virtual ~TestSecond() = default;

    int testFunc1()
    {
        return 987;
    }

    MetaInfo(TestSecond)
    {
        static inline MetaMethod<TestSecond> testFunc1{&BaseType::testFunc1, "testFunc1"};
    };
};

class Mixin : public TestMixin, public TestSecond
{
public:

    MetaInfo(Mixin, TestMixin, TestSecond)
    {
    };

    explicit Mixin()
    {
    }
};

class MetaMethods : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaClass<TestMixin>();
        registerMetaClass<TestSecond>();
        registerMetaClass<Mixin>();
    }
};

TEST_F(MetaMethods, test_mixin_methods)
{
    const auto* mc = TestMixin::StaticMetaClass::get();
    auto visitor = [](const auto, const auto& meta) -> bool
    {
        return meta.name() == "testFunc1";
    };
    auto method = mc->visitMethods(visitor);
    EXPECT_TRUE(method != nullptr);

    method = mc->visitMethods([](const auto, const auto& meta) -> bool { return meta.name() == "whatever"; });
    EXPECT_TRUE(method == nullptr);
}

TEST_F(MetaMethods, test_invoke_undeclared_method)
{
    TestMixin mixin;

    EXPECT_FALSE(metainfo::invoke(mixin, "whatever"));
}

TEST_F(MetaMethods, test_mixin_method_invoke_directly)
{
    TestMixin mixin;
    const TestMixin::StaticMetaClass* metaClass = dynamic_cast<const TestMixin::StaticMetaClass*>(TestMixin::StaticMetaClass::get());
    EXPECT_NOT_NULL(metaClass);

//    metaClass->invoke(mixin, metaClass->testFunc1);
}

TEST_F(MetaMethods, test_mixin_method_invoke_by_method_name)
{
    TestMixin mixin;

    metainfo::invoke(mixin, "testFunc1");
    EXPECT_TRUE(mixin.invoked);

    auto ret = metainfo::invoke(mixin, "testFunc2");
    EXPECT_TRUE(ret);
    EXPECT_EQ(1234321, *ret);
}

TEST_F(MetaMethods, test_mixin_static_method_invoke)
{
    TestMixin mixin;

    auto ret = metainfo::invoke(mixin, "staticFunc", 11);
    EXPECT_TRUE(ret);
    EXPECT_EQ(11, *ret);
}

TEST_F(MetaMethods, test_mixin_invoke_lambda)
{
    TestMixin mixin;
    metainfo::invoke(mixin, "lambda", &mixin);
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_metamethod)
{
    Mixin mixin;
    metainfo::invoke(mixin, "lambda", static_cast<TestMixin*>(&mixin));
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_method_defined_in_superclass)
{
    Mixin mixin;
    auto ret = metainfo::invoke(mixin, "testFunc2");
    EXPECT_TRUE(ret);
    EXPECT_EQ(1234321, *ret);
}

TEST_F(MetaMethods, test_mixin_same_name_methods)
{
    Mixin mixin;
    auto ret = metainfo::invoke(mixin, "testFunc1");
    EXPECT_TRUE(ret);
    // The method lookup uses firt hit, and returns the method that is having no return type,
    // from TestMixin.
    EXPECT_FALSE(ret->isValid());
    // To make sure we call the method defined in TestSecond, we must force the instance type
    ret = metainfo::invoke(static_cast<TestSecond&>(mixin), "testFunc1");
    EXPECT_TRUE(ret);
    EXPECT_EQ(Metatype::Int32, ret->metaType());
    EXPECT_EQ(987, *ret);
}

TEST_F(MetaMethods, test_invoked_with_convertible_arguments)
{
    Mixin mixin;
    auto ret = metainfo::invoke(mixin, "staticFunc", std::string("987"));
    EXPECT_TRUE(ret);
    EXPECT_EQ(987, *ret);
    ret = metainfo::invoke(mixin, "staticFunc", 123.2f);
    EXPECT_TRUE(ret);
    EXPECT_EQ(123, *ret);
}
