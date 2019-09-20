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
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metaobject.hpp>
#include <mox/metadata/callable.hpp>

using namespace mox;

class TestMixin
{
public:
    bool invoked = false;

    virtual ~TestMixin() = default;

    struct StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, TestMixin>
    {
        Method testFunc1{*this, &BaseType::testFunc1, "testFunc1"};
        Method testFunc2{*this, &BaseType::testFunc2, "testFunc2"};
        Method staticFunc{*this, &BaseType::staticFunc, "staticFunc"};
        Method lambda{*this, [](TestMixin* instance) { instance->invoked = true; }, "lambda"};
    };

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
};

class TestSecond
{
public:

    virtual ~TestSecond() = default;

    struct StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, TestSecond>
    {
        Method testFunc1{*this, &BaseType::testFunc1, "testFunc1"};
    };

    int testFunc1()
    {
        return 987;
    }
};

class Mixin : public TestMixin, public TestSecond
{
public:

    struct StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, Mixin, TestMixin, TestSecond>
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
    const MetaClass* mc = TestMixin::StaticMetaClass::get();
    auto visitor = [](const MetaClass::Method* method) -> bool
    {
        return method->name() == "testFunc1";
    };
    const MetaClass::Method* method = mc->visitMethods(visitor);
    EXPECT_TRUE(method != nullptr);

    method = mc->visitMethods([](const MetaClass::Method* method) -> bool { return method->name() == "whatever"; });
    EXPECT_TRUE(method == nullptr);
}

TEST_F(MetaMethods, test_invoke_undeclared_method)
{
    TestMixin mixin;

    EXPECT_FALSE(metaInvoke(mixin, "whatever"));
    // Force the return type of a void function to int.
    int ret = -1;
    EXPECT_FALSE(metaInvoke(mixin, ret, "testFunc1"));
}

TEST_F(MetaMethods, test_mixin_method_invoke_directly)
{
    TestMixin mixin, *ptrMixin = &mixin;
    const TestMixin::StaticMetaClass* metaClass = dynamic_cast<const TestMixin::StaticMetaClass*>(TestMixin::StaticMetaClass::get());
    EXPECT_NOT_NULL(metaClass);

    invoke(metaClass->testFunc1, ptrMixin);
}

TEST_F(MetaMethods, test_mixin_method_invoke_by_method_name)
{
    TestMixin mixin;

    metaInvoke(mixin, "testFunc1");
    EXPECT_TRUE(mixin.invoked);

    int ret = -1;
    EXPECT_TRUE(metaInvoke(mixin, ret, "testFunc2"));
    EXPECT_EQ(1234321, ret);
}

TEST_F(MetaMethods, test_mixin_static_method_invoke)
{
    TestMixin mixin;

    int ret = -1;
    EXPECT_TRUE(metaInvoke(mixin, ret, "staticFunc", 11));
    EXPECT_EQ(11, ret);
}

TEST_F(MetaMethods, test_mixin_invoke_lambda)
{
    TestMixin mixin;
    metaInvoke(mixin, "lambda", &mixin);
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_metamethod)
{
    Mixin mixin;
    metaInvoke(mixin, "lambda", static_cast<TestMixin*>(&mixin));
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_method_defined_in_superclass)
{
    Mixin mixin;
    int ret = -1;
    EXPECT_TRUE(metaInvoke(mixin, ret, "testFunc2"));
    EXPECT_EQ(1234321, ret);
}

TEST_F(MetaMethods, test_mixin_same_name_methods)
{
    Mixin mixin;
    int ret = -1;
    EXPECT_TRUE(metaInvoke(mixin, ret, "testFunc1"));
    EXPECT_EQ(987, ret);
}

TEST_F(MetaMethods, test_invoked_with_convertible_arguments)
{
    Mixin mixin;
    int ret = -1;
    EXPECT_TRUE(metaInvoke(mixin, ret, "staticFunc", std::string("987")));
    EXPECT_EQ(987, ret);
    EXPECT_TRUE(metaInvoke(mixin, ret, "staticFunc", 123.2f));
    EXPECT_EQ(123, ret);
}
