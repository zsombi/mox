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
#include <mox/signal/signal.hpp>

#include <string_view>

using namespace mox;

class TestEmitterNoMetaClass : public ObjectLock
{
public:
    explicit TestEmitterNoMetaClass() = default;

    static inline SignalTypeDecl<> VoidSignalType;
    static inline SignalTypeDecl<int> IntSignalType;

    SignalDecl<> voidSig{*this, VoidSignalType};
    SignalDecl<int> intSig{*this, IntSignalType};
};

class TestEmitterWithMetaClass : public ObjectLock
{
public:
    explicit TestEmitterWithMetaClass() = default;

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, TestEmitterWithMetaClass>
    {
        static inline SignalTypeDecl<> VoidSignalType;
        Signal voidSig{*this, VoidSignalType, "voidSig"};
    };

    static inline SignalTypeDecl<std::string_view> StringSignalType;

    SignalDecl<std::string_view> string{*this, StringSignalType};
    SignalDecl<> voidSig{*this, StaticMetaClass::VoidSignalType};
};

class SignalTestClass : public MetaObject
{
public:
    static inline SignalTypeDecl<> Sign1Des;
    static inline SignalTypeDecl<> SignBDes;
    static inline SignalTypeDecl<int32_t> Sign2Des;
    static inline SignalTypeDecl<int32_t, std::string> Sign3Des;

    SignalDecl<> sig1{*this, Sign1Des};
    SignalDecl<int32_t> sig2{*this, Sign2Des};
    SignalDecl<int32_t, std::string> sig3{*this, Sign3Des};
    SignalDecl<> sigB{*this, SignBDes};

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, SignalTestClass, MetaObject>
    {
        Signal sig1{*this, Sign1Des, "sig1"};
        Signal sigB{*this, SignBDes, "sigB"};
        Signal sig2{*this, Sign2Des, "sig2"};
        Signal sig3{*this, Sign3Des, "sig3"};
    };
};

class DerivedEmitter : public SignalTestClass
{
public:
    static inline SignalTypeDecl<std::vector<int32_t>> SignVDes;
    SignalDecl<std::vector<int32_t>> sigV{*this, SignVDes};

    struct MOX_API StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, DerivedEmitter, SignalTestClass>
    {
        Signal sigV{*this, SignVDes, "sigV"};
    };
};

class  SlotHolder : public ObjectLock
{
    int slot1Call = 0;
    int slot2Call = 0;
    int slot3Call = 0;
    int slot4Call = 0;

public:
    static inline SignalTypeDecl<int> SigDes;
    SignalDecl<int> sig{*this, SigDes};

    struct MOX_API StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, SlotHolder>
    {
        Method method1{*this, &BaseType::method1, "method1"};
        Method method2{*this, &BaseType::method2, "method2"};
        Method method3{*this, &BaseType::method3, "method3"};
        Method method4{*this, &BaseType::method4, "method4"};
        Method autoDisconnect1{*this, &BaseType::autoDisconnect1, "autoDisconnect1"};
        Method autoDisconnect2{*this, &BaseType::autoDisconnect2, "autoDisconnect2"};
        Signal sig{*this, SigDes, "sig"};
    };


    void method1()
    {
        slot1Call++;
    }

    void method2(int32_t v)
    {
        UNUSED(v);
        slot2Call++;
    }

    void method3(int32_t, std::string)
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

    void autoDisconnect1()
    {
        Signal::Connection::getActiveConnection()->disconnect();
    }
    void autoDisconnect2(int32_t v)
    {
        if (v == 10)
        {
            Signal::Connection::getActiveConnection()->disconnect();
        }
    }
};

class DerivedHolder : public SlotHolder
{
    int derived1Call = 0;
    int derived2Call = 0;
public:
    struct MOX_API StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, DerivedHolder, SlotHolder>
    {
        Method derivedMethod1{*this, &BaseType::derivedMethod1, "derivedMethod1"};
        Method derivedMethod2{*this, &BaseType::derivedMethod2, "derivedMethod2"};
    };

    void derivedMethod1()
    {
        derived1Call++;
    }
    void derivedMethod2(int32_t v)
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

void slotFunction2(int32_t)
{
}

class SignalTest: public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaType<std::vector<int32_t>>();
        registerMetaType<TestEmitterNoMetaClass>();
        registerMetaType<TestEmitterNoMetaClass*>();
        registerMetaClass<TestEmitterWithMetaClass>();
        registerMetaClass<SignalTestClass>();
        registerMetaClass<SlotHolder>();
        registerMetaClass<DerivedHolder>();
        registerMetaClass<DerivedEmitter>();
    }
};

TEST_F(SignalTest, test_signal_without_metaclass)
{
    TestEmitterNoMetaClass test;

    EXPECT_EQ(0, test.voidSig());
    EXPECT_EQ(0, test.intSig(10));

    EXPECT_EQ(0, TestEmitterNoMetaClass::VoidSignalType.activate(intptr_t(&test), Callable::ArgumentPack()));
    EXPECT_EQ(0, TestEmitterNoMetaClass::IntSignalType.activate(intptr_t(&test), Callable::ArgumentPack(100)));
}

TEST_F(SignalTest, test_signal_with_metaclass)
{
    TestEmitterWithMetaClass test;

    EXPECT_EQ(0, test.voidSig());
    EXPECT_EQ(0, TestEmitterWithMetaClass::StaticMetaClass::VoidSignalType.activate(intptr_t(&test), Callable::ArgumentPack()));

    auto param = "alpha"sv;
    EXPECT_EQ(0, test.string(param));
    EXPECT_EQ(0, TestEmitterWithMetaClass::StringSignalType.activate(intptr_t(&test), Callable::ArgumentPack(param)));
}


TEST_F(SignalTest, test_signal_api)
{
    SignalTestClass test;
}

TEST_F(SignalTest, test_connect_method)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connect(slots, &SlotHolder::method1));
    EXPECT_NOT_NULL(host.sig2.connect(slots, &SlotHolder::method1));
    EXPECT_NOT_NULL(host.sig2.connect(slots, &SlotHolder::method4));
}

TEST_F(SignalTest, test_connect_metamethod)
{
    SignalTestClass host;
    SlotHolder slots;

    EXPECT_NOT_NULL(host.sig1.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method2"));
    EXPECT_NULL(host.sig2.connect(slots, "method3"));
    EXPECT_NOT_NULL(host.sig2.connect(slots, "method4"));

    EXPECT_NOT_NULL(host.sig3.connect(slots, "method1"));
    EXPECT_NOT_NULL(host.sig3.connect(slots, "method2"));
    EXPECT_NOT_NULL(host.sig3.connect(slots, "method3"));
    EXPECT_NOT_NULL(host.sig3.connect(slots, "method4"));
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
    EXPECT_NOT_NULL(host.sig3.connect(lambda3));
    EXPECT_NOT_NULL(host.sig2.connect(lambda3));

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
    EXPECT_NOT_NULL(emitter.sig3.connect(emitter.sig2));

    EXPECT_EQ(2, emitter.sig3(10, std::string("apple")));
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

    auto fn1 = [](){};

    Signal::ConnectionSharedPtr connection = sender.sig1.connect(fn1);
    EXPECT_NOT_NULL(connection);
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

void autoDisconnect(int32_t v)
{
    if (v == 2)
    {
        Signal::Connection::getActiveConnection()->disconnect();
    }
}

struct TestFunctor
{
    SignalTestClass* sender;
    void explicitDisconnect(int32_t v)
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

    auto lambda = []()
    {
        Signal::Connection::getActiveConnection()->disconnect();
    };

    EXPECT_NOT_NULL(sender.sig2.connect(lambda));
    EXPECT_EQ(1, sender.sig2(1));
    EXPECT_EQ(0, sender.sig2(1));
}

TEST_F(SignalTest, test_signal_in_derived)
{
    DerivedEmitter sender;
    SignalTestClass receiver1;
    SlotHolder receiver2;

    EXPECT_NOT_NULL(sender.sigV.connect(receiver1.sig1));
    EXPECT_NOT_NULL(sender.sig1.connect(receiver1.sig1));
    Signal::ConnectionSharedPtr connection = sender.sigV.connect(receiver2, &SlotHolder::method1);
    EXPECT_NOT_NULL(connection);

    EXPECT_EQ(2, sender.sigV(std::vector<int>()));
    EXPECT_EQ(1, sender.sig1());
}

TEST_F(SignalTest, test_disconnect_next_connection_in_activation)
{
    DerivedEmitter sender;
    SlotHolder receiver;

    auto lambda = [&receiver]()
    {
        Signal* signal = Signal::Connection::getActiveConnection()->signal();
        if (signal)
        {
            signal->disconnect(receiver, &SlotHolder::method1);
        }
    };
    EXPECT_NOT_NULL(sender.sigV.connect(lambda));
    EXPECT_NOT_NULL(sender.sigV.connect(receiver, &SlotHolder::method1));

    // There should be only 1 activation, as lambda disconnects the other connection.
    EXPECT_EQ(1, sender.sigV(std::vector<int>()));
}

TEST_F(SignalTest, test_emit_metasignals)
{
    SignalTestClass sender;

    EXPECT_EQ(0, emit(sender, "sig1"));

    // Invoke with convertible args.
    EXPECT_EQ(0, emit(sender, "sig2", std::string_view("10")));

    // Invoke with not enough args.
    EXPECT_EQ(-1, emit(sender, "sig3", 10));
    EXPECT_EQ(0, emit(sender, "sig3", 10, std::string_view("123")));

    // Invoke a non-existent signal.
    EXPECT_EQ(-1, emit(sender, "sigV"));
}

TEST_F(SignalTest, test_metaclass_invoke_metasignals)
{
    SignalTestClass sender;
    const SignalTestClass::StaticMetaClass* mc = SignalTestClass::StaticMetaClass::get();

    EXPECT_EQ(0, mc->sig1.emit(sender));

    // Invoke with convertible arguments.
    EXPECT_EQ(0, mc->sig2.emit(sender, std::string_view("10")));

    // Invoke with not enough arguments.
    EXPECT_EQ(-1, mc->sig2.emit(sender));
}
