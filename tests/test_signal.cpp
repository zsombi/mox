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
#include <mox/metadata/signal.hpp>

using namespace mox;

class SignalTestClass : public SignalHost
{
public:
    MIXIN_METACLASS_BASE(SignalTestClass)
    {
    };

    Signal<void()> sig1{*this};
    Signal<void(int)> sig2{*this};
    Signal<void(int, std::string)> sig3{*this};
};

class SlotHolder : public SignalHost
{
public:
    MIXIN_METACLASS_BASE(SlotHolder)
    {
        META_METHOD(SlotHolder, method1);
        META_METHOD(SlotHolder, method2);
        META_METHOD(SlotHolder, method3);
        META_METHOD(SlotHolder, method4);
    };

    Signal<void(int)> sig{*this};

    void method1()
    {
    }

    void method2(int v)
    {
        UNUSED(v);
    }

    void method3(int, std::string)
    {
    }

    void method4(float)
    {
    }
};

void slotFunction1()
{
}

void slotFunction2(int)
{
}

class SignalTest: public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaType<SignalTestClass>();
        registerMetaType<SlotHolder>();
    }
};

TEST_F(SignalTest, test_connect_method)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connect(slots, &SlotHolder::method1));
    EXPECT_NOT_NULL(host.sig2.connect(slots, &SlotHolder::method1));
    EXPECT_NULL(host.sig2.connect(slots, &SlotHolder::method4));
}

TEST_F(SignalTest, test_connect_metamethod)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connectMethod(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connectMethod(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connectMethod(slots, "method2"));
    EXPECT_NULL(host.sig2.connectMethod(slots, "method3"));
    EXPECT_NULL(host.sig2.connectMethod(slots, "method4"));

    EXPECT_NOT_NULL(host.sig3.connectMethod(slots, "method1"));
    EXPECT_NOT_NULL(host.sig3.connectMethod(slots, "method2"));
    EXPECT_NOT_NULL(host.sig3.connectMethod(slots, "method3"));
    EXPECT_NULL(host.sig3.connectMethod(slots, "method4"));
}

TEST_F(SignalTest, test_connect_function)
{
    SignalTestClass host;

    EXPECT_NOT_NULL(host.sig1.connect(slotFunction1));
    EXPECT_NULL(host.sig1.connect(slotFunction2));

    EXPECT_NOT_NULL(host.sig2.connect(slotFunction1));
    EXPECT_NOT_NULL(host.sig2.connect(slotFunction2));

    EXPECT_NOT_NULL(host.sig3.connect(slotFunction1));
    EXPECT_NOT_NULL(host.sig3.connect(slotFunction2));
}

TEST_F(SignalTest, test_connect_lambda)
{
    SignalTestClass host;

    auto lambda1 = []() {};

    EXPECT_NOT_NULL(host.sig1.connect(lambda1));
    EXPECT_NOT_NULL(host.sig2.connect(lambda1));
    EXPECT_NOT_NULL(host.sig3.connect(lambda1));

    auto lambda2 = [](int) {};
    EXPECT_NULL(host.sig1.connect(lambda2));
    EXPECT_NOT_NULL(host.sig2.connect(lambda2));
    EXPECT_NOT_NULL(host.sig3.connect(lambda2));

    auto lambda3 = [](float) {};
    EXPECT_NULL(host.sig1.connect(lambda3));
    EXPECT_NULL(host.sig2.connect(lambda3));
    EXPECT_NULL(host.sig3.connect(lambda3));

    auto lambda4 = [](int, std::string) {};
    EXPECT_NULL(host.sig1.connect(lambda4));
    EXPECT_NULL(host.sig2.connect(lambda4));
    EXPECT_NOT_NULL(host.sig3.connect(lambda4));
}

TEST_F(SignalTest, test_connect_signal)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    EXPECT_NULL(emitter.sig1.connect(receiver.sig));
    EXPECT_NOT_NULL(emitter.sig2.connect(receiver.sig));
    EXPECT_NOT_NULL(emitter.sig3.connect(receiver.sig));
}

TEST_F(SignalTest, test_disconnect)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    auto connection = emitter.sig1.connect(receiver, &SlotHolder::method1);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_TRUE(connection->disconnect());
    EXPECT_FALSE(connection->isConnected());
}

TEST_F(SignalTest, test_emit_signal)
{

}

TEST_F(SignalTest, test_disconnect_on_emit)
{

}

TEST_F(SignalTest, test_delete_on_emit)
{

}

TEST_F(SignalTest, test_delete_connection)
{

}

TEST_F(SignalTest, test_delete_signal_source)
{

}

TEST_F(SignalTest, test_delete_connection_destination)
{

}

TEST_F(SignalTest, test_connect_in_emit)
{

}
