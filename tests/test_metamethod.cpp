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

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, TestMixin>
    {
        Method testFunc1{*this, &BaseType::testFunc1, "testFunc1"};
        Method testFunc2{*this, &BaseType::testFunc2, "testFunc2"};
        Method staticFunc{*this, &BaseType::staticFunc, "staticFunc"};
        Method lambda{*this, [](TestMixin* instance) { instance->invoked = true; }, "lambda"};

        MetaClassDefs()
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

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, TestSecond>
    {
        Method testFunc1{*this, &BaseType::testFunc1, "testFunc1"};

        MetaClassDefs()
    };

    int testFunc1()
    {
        return 987;
    }
};

class Mixin : public TestMixin, public TestSecond
{
public:

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, Mixin, TestMixin, TestSecond>
    {
        MetaClassDefs()
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

    EXPECT_FALSE(invoke(mixin, "whatever"));
}

TEST_F(MetaMethods, test_mixin_method_invoke_directly)
{
    TestMixin mixin;
    const TestMixin::StaticMetaClass* metaClass = dynamic_cast<const TestMixin::StaticMetaClass*>(TestMixin::StaticMetaClass::get());
    EXPECT_NOT_NULL(metaClass);

    metaClass->invoke(mixin, metaClass->testFunc1);
}

TEST_F(MetaMethods, test_mixin_method_invoke_by_method_name)
{
    TestMixin mixin;

    invoke(mixin, "testFunc1");
    EXPECT_TRUE(mixin.invoked);

    auto ret = invoke(mixin, "testFunc2");
    EXPECT_TRUE(ret);
    EXPECT_EQ(1234321, *ret);
}

TEST_F(MetaMethods, test_mixin_static_method_invoke)
{
    TestMixin mixin;

    auto ret = invoke(mixin, "staticFunc", 11);
    EXPECT_TRUE(ret);
    EXPECT_EQ(11, *ret);
}

TEST_F(MetaMethods, test_mixin_invoke_lambda)
{
    TestMixin mixin;
    invoke(mixin, "lambda", &mixin);
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_metamethod)
{
    Mixin mixin;
    invoke(mixin, "lambda", static_cast<TestMixin*>(&mixin));
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_method_defined_in_superclass)
{
    Mixin mixin;
    auto ret = invoke(mixin, "testFunc2");
    EXPECT_TRUE(ret);
    EXPECT_EQ(1234321, *ret);
}

TEST_F(MetaMethods, test_mixin_same_name_methods)
{
    Mixin mixin;
    auto ret = invoke(mixin, "testFunc1");
    EXPECT_TRUE(ret);
    // The method lookup uses firt hit, and returns the method that is having no return type,
    // from TestMixin.
    EXPECT_FALSE(ret->isValid());
    // To make sure we call the method defined in TestSecond, we must force the instance type
    ret = invoke(static_cast<TestSecond&>(mixin), "testFunc1");
    EXPECT_TRUE(ret);
    EXPECT_EQ(Metatype::Int32, ret->metaType());
    EXPECT_EQ(987, *ret);
}

TEST_F(MetaMethods, test_invoked_with_convertible_arguments)
{
    Mixin mixin;
    auto ret = invoke(mixin, "staticFunc", std::string("987"));
    EXPECT_TRUE(ret);
    EXPECT_EQ(987, *ret);
    ret = invoke(mixin, "staticFunc", 123.2f);
    EXPECT_TRUE(ret);
    EXPECT_EQ(123, *ret);
}
