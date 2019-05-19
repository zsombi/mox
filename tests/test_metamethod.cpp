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
#include <mox/metadata/metamethod.hpp>
#include <mox/metadata/callable.hpp>

using namespace mox;

class TestMixin
{
public:
    bool invoked = false;

    MIXIN_METACLASS_BASE(TestMixin)
    {
        META_METHOD(TestMixin, testFunc1);
        META_METHOD(TestMixin, testFunc2);
        META_METHOD(TestMixin, staticFunc);
        MetaMethod lambda = {*this, [](TestMixin* instance) { instance->invoked = true; }, "lambda"};
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

    MIXIN_METACLASS_BASE(TestSecond)
    {
        META_METHOD(TestSecond, testFunc1);
    };

    int testFunc1()
    {
        return 987;
    }
};

class Mixin : public TestMixin, public TestSecond
{
public:

    MIXIN_METACLASS(Mixin, TestMixin, TestSecond)
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
    }
};

TEST_F(MetaMethods, test_mixin_methods)
{
    const MetaClass* mc = TestMixin::getStaticMetaClass();
    auto visitor = [](const MetaMethod* method) -> bool
    {
        return method->name() == "testFunc1";
    };
    const MetaMethod* method = mc->visitMethods(visitor);
    EXPECT_TRUE(method != nullptr);

    method = mc->visitMethods([](const MetaMethod* method) -> bool { return method->name() == "whatever"; });
    EXPECT_TRUE(method == nullptr);
}

TEST_F(MetaMethods, test_invoke_undeclared_method)
{
    TestMixin mixin;

    EXPECT_THROW(invokeMethod<void>(mixin, "whatever"), mox::metamethod_not_found);
    // Force the return type of a void function to int.
    EXPECT_THROW(invokeMethod<int>(mixin, "testFunc1"), mox::metamethod_not_found);
}

TEST_F(MetaMethods, test_mixin_method_invoke_directly)
{
    TestMixin mixin, *ptrMixin = &mixin;
    const TestMixin::TestMixinMetaClass* metaClass = dynamic_cast<const TestMixin::TestMixinMetaClass*>(mixin.getStaticMetaClass());
    EXPECT_NOT_NULL(metaClass);

    invoke<void>(metaClass->testFunc1, ptrMixin);
}

TEST_F(MetaMethods, test_mixin_method_invoke_by_method_name)
{
    TestMixin mixin;

    invokeMethod<void>(mixin, "testFunc1");
    EXPECT_TRUE(mixin.invoked);

    EXPECT_EQ(1234321, invokeMethod<int>(mixin, "testFunc2"));
}

TEST_F(MetaMethods, test_mixin_static_method_invoke)
{
    TestMixin mixin;

    EXPECT_EQ(11, invokeMethod<int>(mixin, "staticFunc", 11));
}

TEST_F(MetaMethods, test_mixin_invoke_lambda)
{
    TestMixin mixin;
    invokeMethod<void>(mixin, "lambda", &mixin);
    EXPECT_TRUE(mixin.invoked);
}

TEST_F(MetaMethods, test_mixin_method_defined_in_superclass)
{
    Mixin mixin;
    invokeMethod<void>(mixin, "lambda", static_cast<TestMixin*>(&mixin));
    EXPECT_TRUE(mixin.invoked);

    EXPECT_EQ(1234321, invokeMethod<int>(mixin, "testFunc2"));
}

TEST_F(MetaMethods, test_mixin_same_name_methods)
{
    Mixin mixin;
    EXPECT_EQ(987, invokeMethod<int>(mixin, "testFunc1"));
}
