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
#include <mox/metadata/metaobject.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/signal.hpp>

using namespace mox;

class SignalTestClass : public SignalHost
{
public:
    decl::Signal<void()> sig1{*this, "sig1"};
    decl::Signal<void(int)> sig2{*this, "sig2"};
    decl::Signal<void(int, std::string)> sig3{*this, "sig3"};
    decl::Signal<void()> sigB{*this, "sigB"};

    MIXIN_METACLASS_BASE(SignalTestClass)
    {
        META_SIGNAL(sig1);
        META_SIGNAL(sigB);
        META_SIGNAL(sig2);
        META_SIGNAL(sig3);
    };
};

class  SlotHolder : public SignalHost
{
    int slot1Call = 0;
    int slot2Call = 0;
    int slot3Call = 0;
    int slot4Call = 0;

public:
    decl::Signal<void(int)> sig{*this, "sig"};

    MIXIN_METACLASS_BASE(SlotHolder)
    {
        META_METHOD(SlotHolder, method1);
        META_METHOD(SlotHolder, method2);
        META_METHOD(SlotHolder, method3);
        META_METHOD(SlotHolder, method4);
        META_METHOD(SlotHolder, autoDisconnect1);
        META_METHOD(SlotHolder, autoDisconnect2);
        META_SIGNAL(sig);
    };


    void method1()
    {
        slot1Call++;
    }

    void method2(int v)
    {
        UNUSED(v);
        slot2Call++;
    }

    void method3(int, std::string)
    {
        slot3Call++;
    }

    void method4(float)
    {
        slot4Call++;
    }

    int slot1CallCount() const
    {
        return slot1Call;
    }
    int slot2CallCount() const
    {
        return slot2Call;
    }
    int slot3CallCount() const
    {
        return slot3Call;
    }
    int slot4CallCount() const
    {
        return slot4Call;
    }

    void notMetaMethod()
    {
    }

    void autoDisconnect1(Signal::ConnectionSharedPtr connection)
    {
        connection->disconnect();
    }
    void autoDisconnect2(Signal::ConnectionSharedPtr connection, int v)
    {
        if (v == 10)
        {
            connection->disconnect();
        }
    }
};

class DerivedHolder : public SlotHolder
{
    int derived1Call = 0;
    int derived2Call = 0;
public:
    MIXIN_METACLASS(DerivedHolder, SlotHolder)
    {
        META_METHOD(DerivedHolder, derivedMethod1);
        META_METHOD(DerivedHolder, derivedMethod2);
    };

    void derivedMethod1()
    {
        derived1Call++;
    }
    void derivedMethod2(int v)
    {
        derived2Call = v;
    }

    int derived1CallData()
    {
        return derived1Call;
    }
    int derived2CallData()
    {
        return derived2Call;
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
        registerTestType<SignalTestClass>();
        registerTestType<SlotHolder>();
        registerTestType<DerivedHolder>();
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

    EXPECT_NOT_NULL(host.sig1.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method2"));
    EXPECT_NULL(host.sig2.connect(slots, "method3"));
    EXPECT_NULL(host.sig2.connect(slots, "method4"));

    EXPECT_NOT_NULL(host.sig3.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig3.connect(slots, "method2"));
    EXPECT_NOT_NULL(host.sig3.connect(slots, "method3"));
    EXPECT_NULL(host.sig3.connect(slots, "method4"));
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

TEST_F(SignalTest, test_disconnect_functor)
{
    SignalTestClass sender;

    std::function<void()> fn1 = []() {};

    EXPECT_NOT_NULL(sender.sig1.connect(fn1));
    EXPECT_EQ(1u, sender.sig1());
    sender.sig1.disconnect(fn1);
    EXPECT_EQ(0u, sender.sig1());
}

TEST_F(SignalTest, test_disconnect_function)
{
    SignalTestClass sender;

    sender.sig1.connect(slotFunction1);
    EXPECT_EQ(1u, sender.sig1());
    sender.sig1.disconnect(slotFunction1);
    EXPECT_EQ(0u, sender.sig1());
}

TEST_F(SignalTest, test_disconnect_method)
{
    SignalTestClass sender;
    SlotHolder receiver;

    Signal::ConnectionSharedPtr connection = sender.sig2.connect(receiver, &SlotHolder::method2);
    EXPECT_EQ(1u, sender.sig2(1));

    sender.sig2.disconnect(receiver, &SlotHolder::method2);
    EXPECT_EQ(0u, sender.sig2(1));
}

TEST_F(SignalTest, test_disconnect_signal)
{
    SignalTestClass sender;
    SlotHolder receiver;

    sender.sig2.connect(receiver.sig);
    EXPECT_EQ(1u, sender.sig2(1));
    sender.sig2.disconnect(receiver.sig);
    EXPECT_EQ(0u, sender.sig2(1));
}

TEST_F(SignalTest, test_disconnect_metamethod)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connect(slots, "method1"));
    EXPECT_EQ(1u, host.sig1());
    EXPECT_TRUE(host.sig1.disconnect(slots, "method1"));
    EXPECT_FALSE(host.sig1.disconnect(slots, "method2"));
    EXPECT_EQ(0u, host.sig1());

    EXPECT_NOT_NULL(host.sig2.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method2"));
    EXPECT_EQ(2u, host.sig2(1));

    EXPECT_TRUE(host.sig2.disconnect(slots, "method2"));
    EXPECT_EQ(1u, host.sig2(1));
}

TEST_F(SignalTest, test_connect_as_address_disconnect_as_methodname)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connect(slots, &SlotHolder::method1));
    EXPECT_TRUE(host.sig1.disconnect(slots, "method1"));

    EXPECT_NOT_NULL(host.sig1.connect(slots, "method1"));
    EXPECT_TRUE(host.sig1.disconnect(slots, &SlotHolder::method1));

    EXPECT_NOT_NULL(host.sig1.connect(slots, &SlotHolder::notMetaMethod));
    EXPECT_FALSE(host.sig1.disconnect(slots, "notMetaMethod"));

    EXPECT_NULL(host.sig1.connect(slots, "notMetaMethod"));
}

TEST_F(SignalTest, test_emit_signal)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    EXPECT_EQ(0u, emitter.sig1());
    emitter.sig1.connect(receiver, &SlotHolder::method1);
    EXPECT_EQ(1u, emitter.sig1());
    EXPECT_EQ(1u, receiver.slot1CallCount());
}

TEST_F(SignalTest, test_emit_signal_connected_to_superclass)
{
    SignalTestClass emitter;
    DerivedHolder receiver;

    EXPECT_NOT_NULL(emitter.sig1.connect(receiver, &DerivedHolder::method1));
    EXPECT_EQ(1u, emitter.sig1());
    EXPECT_EQ(1u, receiver.slot1CallCount());

    EXPECT_NOT_NULL(emitter.sig2.connect(receiver, &DerivedHolder::method2));
    EXPECT_NOT_NULL(emitter.sig2.connect(receiver, &DerivedHolder::derivedMethod2));

    EXPECT_EQ(2u, emitter.sig2(10));
    EXPECT_EQ(1u, receiver.slot1CallCount());
    EXPECT_EQ(10u, receiver.derived2CallData());
}

TEST_F(SignalTest, test_emit_signal_connected_to_signal)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    EXPECT_EQ(0u, emitter.sig2(1));
    EXPECT_NOT_NULL(emitter.sig2.connect(receiver.sig));
    EXPECT_EQ(1, emitter.sig2(1));
    EXPECT_EQ(0u, receiver.slot2CallCount());

    EXPECT_NOT_NULL(receiver.sig.connect(receiver, &SlotHolder::method2));
    EXPECT_EQ(1u, emitter.sig2(1));
    EXPECT_EQ(1u, receiver.slot2CallCount());
}

TEST_F(SignalTest, test_emit_signal_with_args)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    EXPECT_EQ(0u, emitter.sig2(10));
    emitter.sig2.connect(receiver, &SlotHolder::method1);
    emitter.sig2.connect(receiver, &SlotHolder::method2);
    EXPECT_EQ(2u, emitter.sig2(10));
    EXPECT_EQ(1u, receiver.slot1CallCount());
    EXPECT_EQ(1u, receiver.slot2CallCount());
}

TEST_F(SignalTest, test_connect_in_emit_excluded_from_activation)
{
    SignalTestClass emitter;
    SlotHolder receiver;

    auto lambda = [&emitter, &receiver](int)
    {
        emitter.sig2.connect(receiver, &SlotHolder::method2);
    };
    EXPECT_NOT_NULL(emitter.sig2.connect(lambda));
    EXPECT_EQ(1u, emitter.sig2(10));
    EXPECT_EQ(0u, receiver.slot2CallCount());
}

TEST_F(SignalTest, test_emit_same_signal_in_slot_dismissed)
{
    SignalTestClass sender;

    auto lambda = [&sender]()
    {
        sender.sig1();
    };

    EXPECT_NOT_NULL(sender.sig1.connect(lambda));
    EXPECT_EQ(1u, sender.sig1());
}

TEST_F(SignalTest, test_disconnect_on_emit)
{
    SignalTestClass sender;
    SlotHolder receiver;

    EXPECT_NOT_NULL(sender.sig1.connect(receiver, &SlotHolder::autoDisconnect1));
    EXPECT_EQ(1, sender.sig1());
    EXPECT_EQ(0, sender.sig1());

    EXPECT_NOT_NULL(sender.sig2.connect(receiver, &SlotHolder::autoDisconnect2));
    EXPECT_EQ(1, sender.sig2(1001));
    EXPECT_EQ(1, sender.sig2(10));
    EXPECT_EQ(0, sender.sig2(1));
    EXPECT_EQ(0, sender.sig2(10));
}

void autoDisconnect(Signal::ConnectionSharedPtr connection, int v)
{
    if (v == 2)
    {
        connection->disconnect();
    }
}

struct TestFunctor
{
    SignalTestClass* sender;
    void explicitDisconnect(int v)
    {
        if (v == 3)
        {
            sender->sig2.disconnect(*this, &TestFunctor::explicitDisconnect);
        }
    }
};

TEST_F(SignalTest, test_disconnect_on_emit_from_function)
{
    SignalTestClass sender;

    EXPECT_NOT_NULL(sender.sig2.connect(autoDisconnect));
    EXPECT_EQ(1, sender.sig2(2));
    EXPECT_EQ(0, sender.sig2(2));
}

TEST_F(SignalTest, test_explicit_disconnect_in_signal_activation)
{
    SignalTestClass sender;
    TestFunctor receiver = {&sender};

    EXPECT_NOT_NULL(sender.sig2.connect(receiver, &TestFunctor::explicitDisconnect));
    EXPECT_EQ(1, sender.sig2(3));
    EXPECT_EQ(0, sender.sig2(3));
}

TEST_F(SignalTest, test_disconnect_on_emit_from_lambda)
{
    SignalTestClass sender;

    auto lambda = [](Signal::ConnectionSharedPtr connection)
    {
        connection->disconnect();
    };

    EXPECT_NOT_NULL(sender.sig2.connect(lambda));
    EXPECT_EQ(1, sender.sig2(1));
    EXPECT_EQ(0, sender.sig2(1));
}

TEST_F(SignalTest, test_metasignal_emit)
{
    SignalTestClass sender;
    SlotHolder receiver;

    const MetaClass* metaClass = sender.getStaticMetaClass();
    auto visitor = [](const MetaSignal* signal)
    {
        return bool(signal->name() == "sig1");
    };
    const MetaSignal* signal = metaClass->visitSignals(visitor);
    EXPECT_NOT_NULL(signal);
    Callable::Arguments args;
    EXPECT_EQ(0u, signal->activate(sender, args));

    sender.sig1.connect(receiver, &SlotHolder::method1);
    EXPECT_EQ(1, sender.sig1());
    EXPECT_EQ(1, signal->activate(sender, args));

    EXPECT_EQ(1, emit("sig1", sender));
    EXPECT_EQ(-1, emit("whatever", sender));
}

TEST_F(SignalTest, test_emit)
{
    SignalTestClass sender;

    EXPECT_EQ(0, emit("sig1", sender));
    EXPECT_EQ(0, emit("sig1", sender, 10, "bla"));
    EXPECT_EQ(-1, emit("sig2", sender));
    EXPECT_EQ(0, emit("sig2", sender, 10));
    EXPECT_EQ(0, emit("sig2", sender, 10, "bla"));
}
