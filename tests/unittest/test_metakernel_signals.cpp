// Copyright (C) 2020 bitWelder

#include "test_framework.h"
#include <mox/utils/log/logger.hpp>
#include <mox/core/meta/argument_data.hpp>
#include <mox/core/meta/signals.hpp>

DECLARE_LOG_CATEGORY(signalTest)
using namespace mox;

class MetakernelSignals : public UnitTest
{
    ScopeLogType<mox::LogType::All> signalLogs{"signalTest"};
public:
    explicit MetakernelSignals() = default;
};

namespace test_signals
{

class TestMethods : public SlotHolder
{
public:
    virtual ~TestMethods() = default;

    void method1()
    {
        CTRACE(signalTest, __FUNCTION__ << "called");
    }

    void method2(int value)
    {
        CTRACE(signalTest, __FUNCTION__ << "called with" << value);
    }

    int method3(int value)
    {
        CTRACE(signalTest, __FUNCTION__ << "called with" << value);
        return -1 * value;
    }

    virtual void virtual1()
    {
        CTRACE(signalTest, __FUNCTION__ << "base called");
    }
};

class TestDerived : public TestMethods
{
public:
    Signal<> memberSignal{*this};

    void virtual1() override
    {
        CTRACE(signalTest, __FUNCTION__ << "derived called");
    }
};

void function1()
{
    CTRACE(signalTest, __FUNCTION__ << "called");
}

void function2(int value)
{
    CTRACE(signalTest, __FUNCTION__ << "called with" << value);
}

int function3(int value)
{
    CTRACE(signalTest, __FUNCTION__ << "called with" << value);
    return -1 * value;
}

class CustomConnection : public Connection
{
    CustomConnection(SignalCore& sender)
        : Connection(sender)
    {
    }

public:
    static ConnectionPtr connect(SignalCore& sender)
    {
        auto connection = mox::make_polymorphic_shared_ptr<Connection>(new CustomConnection(sender));
        sender.addConnection(connection);
        return connection;
    }

protected:
    void invokeOverride(const PackedArguments&) override
    {
        CTRACE(signalTest, "CustomConnection invoked by a signal with" << m_sender->getArgumentCount() << "arguments");
    }
};

}

TEST_F(MetakernelSignals, test_signal_api_no_arguments)
{
    Lockable host;
    Signal<> signal(host);
    EXPECT_EQ(-1, signal());
}

TEST_F(MetakernelSignals, test_signal_api_int_argument)
{
    Lockable host;
    Signal<int> signal(host);
    EXPECT_EQ(-1, signal(10));
}

TEST_F(MetakernelSignals, test_signal_api_int_stringview_argument)
{
    Lockable host;
    Signal<int, std::string_view> signal(host);
    EXPECT_EQ(-1, signal(10, "signal"sv));
}

TEST_F(MetakernelSignals, test_signal_no_args_connected_to_method)
{
    EXPECT_TRACE(signalTest, "method1 called");
    Lockable host;
    Signal<> signal(host);
    test_signals::TestMethods receiver;
    EXPECT_NOT_NULL(signal.connect(receiver, &test_signals::TestMethods::method1));
    EXPECT_EQ(1, signal());

    // Must not compile!
//    signal.connect(receiver, &test_signals::TestMethods::method2);
}

TEST_F(MetakernelSignals, test_signal_int_arg_connected_to_method)
{
    EXPECT_TRACE(signalTest, "method1 called");
    EXPECT_TRACE(signalTest, "method2 called with 101");
    Lockable host;
    Signal<int> signal(host);
    test_signals::TestMethods receiver;
    EXPECT_NOT_NULL(signal.connect(receiver, &test_signals::TestMethods::method1));
    EXPECT_NOT_NULL(signal.connect(receiver, &test_signals::TestMethods::method2));
    EXPECT_EQ(2, signal(101));
}

TEST_F(MetakernelSignals, test_signal_int_arg_connected_to_method_with_return_type)
{
    EXPECT_TRACE(signalTest, "method3 called with 101");
    Lockable host;
    Signal<int> signal(host);
    test_signals::TestMethods receiver;
    EXPECT_NOT_NULL(signal.connect(receiver, &test_signals::TestMethods::method3));
    EXPECT_EQ(1, signal(101));
}

TEST_F(MetakernelSignals, test_signal_no_arg_connect_to_function)
{
    EXPECT_TRACE(signalTest, "function1 called");
    Lockable host;
    Signal<> signal(host);
    EXPECT_NOT_NULL(signal.connect(&test_signals::function1));
    EXPECT_EQ(1, signal());

    // Must not compile!
//    signal.connect(&test_signals::function2);
}

TEST_F(MetakernelSignals, test_signal_int_arg_connect_to_function)
{
    EXPECT_TRACE(signalTest, "function1 called");
    EXPECT_TRACE(signalTest, "function2 called with 1002");
    Lockable host;
    Signal<int> signal(host);
    EXPECT_NOT_NULL(signal.connect(&test_signals::function1));
    EXPECT_NOT_NULL(signal.connect(&test_signals::function2));
    EXPECT_EQ(2, signal(1002));
}

TEST_F(MetakernelSignals, test_signal_int_arg_connect_to_function_with_return_type)
{
    EXPECT_TRACE(signalTest, "function3 called with 1001");
    Lockable host;
    Signal<int> signal(host);
    EXPECT_NOT_NULL(signal.connect(&test_signals::function3));
    EXPECT_EQ(1, signal(1001));
}

TEST_F(MetakernelSignals, test_signal_no_arg_connect_to_lambda)
{
    EXPECT_TRACE(signalTest, "lambda called");
    Lockable host;
    Signal<> signal(host);
    auto lambda = []()
    {
        CTRACE(signalTest, "lambda called");
    };
    EXPECT_NOT_NULL(signal.connect(lambda));
    EXPECT_EQ(1, signal());

    // Must not compile!
//    auto lambda2 = [](int) {};
//    signal.connect(lambda2);
}

TEST_F(MetakernelSignals, test_signal_int_arg_connect_to_lambda)
{
    EXPECT_TRACE(signalTest, "lambda1 called");
    EXPECT_TRACE(signalTest, "lambda2 called with 1002");
    Lockable host;
    Signal<int> signal(host);
    auto lambda1 = []()
    {
        CTRACE(signalTest, "lambda1 called");
    };
    auto lambda2 = [](int value)
    {
        CTRACE(signalTest, "lambda2 called with" << value);
    };
    EXPECT_NOT_NULL(signal.connect(lambda1));
    EXPECT_NOT_NULL(signal.connect(lambda2));
    EXPECT_EQ(2, signal(1002));
}

TEST_F(MetakernelSignals, test_signal_int_arg_connect_to_lambda_with_return_type)
{
    EXPECT_TRACE(signalTest, "lambda called with 1001");
    Lockable host;
    Signal<int> signal(host);
    auto lambda = [](int value)
    {
        CTRACE(signalTest, "lambda called with" << value);
        return -1 * value;
    };
    EXPECT_NOT_NULL(signal.connect(lambda));
    EXPECT_EQ(1, signal(1001));
}


TEST_F(MetakernelSignals, test_signal_connect_to_signal)
{
    EXPECT_TRACE(signalTest, "function1 called");
    Lockable host;
    Signal<> sender(host);
    Signal<> receiver(host);
    EXPECT_NOT_NULL(sender.connect(receiver));
    receiver.connect(&test_signals::function1);
    EXPECT_EQ(1, sender());

    // Must not compile!
//    Signal<int> receiver2;
//    sender.connect(receiver2);
}

TEST_F(MetakernelSignals, test_signal_connect_to_signal_with_compatible_arguments)
{
    EXPECT_TRACE(signalTest, "function1 called");
    Lockable host;
    Signal<int> sender(host);
    Signal<> receiver(host);
    EXPECT_NOT_NULL(sender.connect(receiver));
    receiver.connect(&test_signals::function1);
    EXPECT_EQ(1, sender(10));
}

TEST_F(MetakernelSignals, test_disconnect_connection_using_signal_api)
{
    Lockable host;
    Signal<> signal(host);
    auto lambda = [](){};
    auto connection = signal.connect(lambda);
    EXPECT_NOT_NULL(connection);
    EXPECT_TRUE(connection->isConnected());
    signal.disconnect(connection);
    EXPECT_FALSE(connection->isConnected());
}

TEST_F(MetakernelSignals, test_disconnect_in_slot)
{
    Lockable host;
    Signal<> signal(host);
    auto lambda = []()
    {
        EXPECT_NOT_NULL(Connection::getActiveConnection());
        Connection::getActiveConnection()->disconnect();
    };
    auto connection = signal.connect(lambda);
    EXPECT_NOT_NULL(connection);
    EXPECT_TRUE(connection->isConnected());
    EXPECT_EQ(1, signal());
    EXPECT_FALSE(connection->isConnected());
}

TEST_F(MetakernelSignals, test_new_connection_in_slot)
{
    Lockable host;
    Signal<> signal(host);
    auto lambda = [&signal]()
    {
        auto innerLambda = [](){};
        signal.connect(innerLambda);
    };
    signal.connect(lambda);
    EXPECT_EQ(1, signal());
    // A second signal call would have two connections.
    EXPECT_EQ(2, signal());
    // Any consecutive call increase the emit count.
    EXPECT_EQ(3, signal());
    EXPECT_EQ(4, signal());
    EXPECT_EQ(5, signal());
    EXPECT_EQ(6, signal());
}

TEST_F(MetakernelSignals, test_custom_connection)
{
    EXPECT_TRACE(signalTest, "CustomConnection invoked by a signal with 0 arguments");
    Lockable host;
    Signal<> signal1(host);
    auto connection = test_signals::CustomConnection::connect(signal1);
    EXPECT_NOT_NULL(connection);
    EXPECT_EQ(&signal1, connection->getSignal());
    signal1();
}

TEST_F(MetakernelSignals, test_multipe_signals_connect_to_same_custom_connection)
{
    EXPECT_TRACE(signalTest, "CustomConnection invoked by a signal with 0 arguments");
    EXPECT_TRACE(signalTest, "CustomConnection invoked by a signal with 1 arguments");
    EXPECT_TRACE(signalTest, "CustomConnection invoked by a signal with 2 arguments");
    Lockable host;
    Signal<> signal1(host);
    Signal<int> signal2(host);
    Signal<int, std::string_view> signal3(host);
    test_signals::CustomConnection::connect(signal1);
    test_signals::CustomConnection::connect(signal2);
    test_signals::CustomConnection::connect(signal3);
    signal1();
    signal2(10);
    signal3(11, "custom"sv);
}

TEST_F(MetakernelSignals, test_receiver_destroyed_early)
{
    Lockable host;
    Signal<> signal(host);
    auto receiver = std::make_shared<test_signals::TestMethods>();
    auto connection = std::weak_ptr<Connection>(signal.connect(*receiver, &test_signals::TestMethods::method1));
    EXPECT_TRUE(connection.lock()->isConnected());

    receiver.reset();
    ASSERT_FALSE(connection.expired());
    EXPECT_FALSE(connection.lock()->isConnected());
}

TEST_F(MetakernelSignals, test_sender_destroyed_early)
{
    Lockable host;
    auto signal = std::make_shared<Signal<>>(host);
    test_signals::TestMethods receiver;
    auto connection = std::weak_ptr<Connection>(signal->connect(receiver, &test_signals::TestMethods::method1));
    EXPECT_TRUE(connection.lock()->isConnected());

    signal.reset();
    EXPECT_TRUE(connection.expired());
}

TEST_F(MetakernelSignals, test_signal_connected_to_virtual_method)
{
    EXPECT_TRACE(signalTest, "virtual1 base called");
    EXPECT_TRACE(signalTest, "virtual1 derived called");
    Lockable host;
    Signal<> signal(host);
    test_signals::TestDerived receiver;
    test_signals::TestMethods base;
    signal.connect(receiver, &test_signals::TestDerived::virtual1);
    signal.connect(base, &test_signals::TestMethods::virtual1);
    EXPECT_EQ(2, signal());
}

TEST_F(MetakernelSignals, test_signal_member_connect_to_method_in_other_object)
{
    EXPECT_TRACE(signalTest, "method1 called");
    test_signals::TestDerived sender;
    test_signals::TestMethods receiver;

    sender.memberSignal.connect(receiver, &test_signals::TestMethods::method1);
    EXPECT_EQ(1, sender.memberSignal());
}
