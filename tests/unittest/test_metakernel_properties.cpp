// Copyright (C) 2020 bitWelder

#include "test_framework.h"
#include <mox/utils/log/logger.hpp>
#include <mox/core/metakernel/argument_data.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/core/metakernel/properties.hpp>

DECLARE_LOG_CATEGORY(propertyTest)
using namespace mox;

class MetakernelProperties : public UnitTest
{
    ScopeLogType<mox::LogType::All> signalLogs{CATEGORY(propertyTest)};
};

namespace test_property
{

enum class TestEnum
{
    One,
    Two,
    Three
};

class CustomDP : public metakernel::StatusProperty<int>::Data
{
    int m_data = -1;

public:
    CustomDP() = default;
    CustomDP(int defValue)
        : m_data(defValue)
    {
    }
    int get() const override
    {
        return m_data;
    }
    void set(int data)
    {
        m_data = data;
        update();
    }
};

template <class Type>
class TestStatus : public metakernel::StatusProperty<Type>::Data, public metakernel::StatusProperty<Type>
{
    Type m_data;

    Type get() const override
    {
        return m_data;
    }

public:
    TestStatus(metakernel::Lockable& host, Type defValue)
        : metakernel::StatusProperty<Type>(host, static_cast<typename metakernel::StatusProperty<Type>::Data&>(*this))
        , m_data(defValue)
    {
    }

    void setData(Type data)
    {
        m_data = data;
        metakernel::StatusProperty<Type>::Data::update();
    }
};

template <class ValueType>
class TestHost : public metakernel::Lockable
{
public:
    explicit TestHost(ValueType defValue = ValueType())
    {
        property1 = defValue;
    }

    metakernel::Property<ValueType> property1{*this};
    metakernel::Property<ValueType> property2{*this};
    metakernel::Property<ValueType> property3{*this};
    metakernel::Property<ValueType> property4{*this};
};

}

TEST_F(MetakernelProperties, test_property_api)
{
    metakernel::Lockable host;
    metakernel::Property<int> property(host);
    EXPECT_EQ(0, property);

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    property.changed.connect(onPropertyChanged);
    EXPECT_TRACE(propertyTest, "Property value changed to 10");
    property = 10;
    EXPECT_EQ(10, property);
}

TEST_F(MetakernelProperties, test_status_property)
{
    EXPECT_TRACE(propertyTest, "Property value changed to 1");
    metakernel::Lockable host;
    test_property::CustomDP datadProvider;
    metakernel::StatusProperty<int> property(host, datadProvider);

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    property.changed.connect(onPropertyChanged);
    EXPECT_EQ(-1, property);

    datadProvider.set(1);
}

TEST_F(MetakernelProperties, test_writable_enum_property)
{
    metakernel::Lockable host;
    metakernel::Property<test_property::TestEnum> property(host, test_property::TestEnum::Two);
    EXPECT_EQ(test_property::TestEnum::Two, property);

    property = test_property::TestEnum::Three;
    EXPECT_EQ(test_property::TestEnum::Three, property);
}

TEST_F(MetakernelProperties, test_enum_status_property)
{
    metakernel::Lockable host;
    test_property::TestStatus<test_property::TestEnum> property(host, test_property::TestEnum::Two);
    EXPECT_EQ(test_property::TestEnum::Two, property);

    property.setData(test_property::TestEnum::Three);
    EXPECT_EQ(test_property::TestEnum::Three, property);
}

TEST_F(MetakernelProperties, test_member_writable_property)
{
    test_property::TestHost<int> test;
    EXPECT_EQ(0, test.property1);

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    test.property1.changed.connect(onPropertyChanged);
    EXPECT_TRACE(propertyTest, "Property value changed to 10");
    test.property1 = 10;
    EXPECT_EQ(10, test.property1);
}

TEST_F(MetakernelProperties, test_one_way_binding_on_same_host_discard_on_write)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);

    auto binding = property1.bind(property2);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(2, property1);
    property2 = 10;
    EXPECT_EQ(10, property1);
    EXPECT_TRUE(binding->isAttached());

    // break the binding by writing to property1
    property1 = 3;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_EQ(3, property1);
    EXPECT_EQ(10, property2);
}

TEST_F(MetakernelProperties, test_one_way_binding_on_separate_hosts_discard_on_write)
{
    metakernel::Lockable host1;
    metakernel::Lockable host2;
    metakernel::Property<int> property1(host1, 1);
    metakernel::Property<int> property2(host2, 2);

    auto binding = property1.bind(property2);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(2, property1);
    property2 = 10;
    EXPECT_EQ(10, property1);
    EXPECT_TRUE(binding->isAttached());

    // break the binding by writing to property1
    property1 = 3;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_EQ(3, property1);
    EXPECT_EQ(10, property2);
}

TEST_F(MetakernelProperties, test_one_way_binding_on_member_properties_on_same_host_discard_on_write)
{
    test_property::TestHost<int> host;
    host.property1 = 1;
    host.property2 = 2;

    auto binding = host.property1.bind(host.property2);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(2, host.property1);
    host.property2 = 10;
    EXPECT_EQ(10, host.property1);
    EXPECT_TRUE(binding->isAttached());

    // break the binding by writing to property1
    host.property1 = 3;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_EQ(3, host.property1);
    EXPECT_EQ(10, host.property2);
}

TEST_F(MetakernelProperties, test_one_way_binding_on_member_properties_on_separate_host_discard_on_write)
{
    test_property::TestHost<int> host1(1);
    test_property::TestHost<int> host2(2);

    auto binding = host1.property1.bind(host2.property1);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(2, host1.property1);
    host2.property1 = 10;
    EXPECT_EQ(10, host1.property1);
    EXPECT_TRUE(binding->isAttached());

    // break the binding by writing to property1
    host1.property1 = 3;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_EQ(3, host1.property1);
    EXPECT_EQ(10, host2.property1);
}

TEST_F(MetakernelProperties, test_one_way_binding_on_same_host_keep_on_write)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);

    auto binding = property1.bind(property2, metakernel::BindingPolicy::KeepOnWrite);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(2, property1);

    property2 = 10;
    EXPECT_EQ(10, property1);
    EXPECT_TRUE(binding->isAttached());

    // the binding is not broken by writing to property1
    property1 = 3;
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(3, property1);
    EXPECT_EQ(10, property2);

    property2 = 5;
    EXPECT_EQ(5, property1);
}

TEST_F(MetakernelProperties, test_grouped_bindings_discard_group_when_binding_is_detached)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);

    metakernel::BindingGroup::create()->addToGroup(*property1.bind(property2)).addToGroup(*property2.bind(property1)).setPolicy(metakernel::BindingPolicy::DetachOnWrite);
    EXPECT_EQ(2, property1);
    EXPECT_EQ(2, property2);

    property1 = 3;
    EXPECT_EQ(3, property1);
    EXPECT_EQ(2, property2);
}

TEST_F(MetakernelProperties, test_two_way_binding_of_2_properties_grouped)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);
    EXPECT_EQ(1, property1);
    EXPECT_EQ(2, property2);

    metakernel::bindProperties(property1, property2);
    EXPECT_EQ(2, property1);
    EXPECT_EQ(2, property2);

    // change one of the property at a time
    auto p1c = int(0);
    auto p2c = int(0);
    auto onP1Changed = [&p1c]() { ++p1c; };
    auto onP2Changed = [&p2c]() { ++p2c; };
    property1.changed.connect(onP1Changed);
    property2.changed.connect(onP2Changed);

    property1 = 100;
    EXPECT_EQ(100, property1);
    EXPECT_EQ(100, property2);
    EXPECT_EQ(1, p1c);
    EXPECT_EQ(1, p2c);

    property2 = 200;
    EXPECT_EQ(200, property1);
    EXPECT_EQ(200, property2);
}

TEST_F(MetakernelProperties, test_bind_3_properties)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);
    metakernel::Property<int> property3(host, 3);

    metakernel::bindProperties(property1, property2, property3);
    EXPECT_EQ(3, property1);
    EXPECT_EQ(3, property2);
    EXPECT_EQ(3, property3);

    auto p1c = int(0);
    auto p2c = int(0);
    auto p3c = int(0);
    auto onP1Changed = [&p1c]() { ++p1c; };
    auto onP2Changed = [&p2c]() { ++p2c; };
    auto onP3Changed = [&p3c]() { ++p3c; };
    property1.changed.connect(onP1Changed);
    property2.changed.connect(onP2Changed);
    property3.changed.connect(onP3Changed);

    property3 = 101;
    EXPECT_EQ(101, property1);
    EXPECT_EQ(101, property2);
    EXPECT_EQ(101, property3);
}

TEST_F(MetakernelProperties, test_bind_4_properties_in_loop)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);
    metakernel::Property<int> property3(host, 3);
    metakernel::Property<int> property4(host, 4);

    metakernel::bindProperties(property1, property2, property3, property4);
    EXPECT_EQ(4, property1);
    EXPECT_EQ(4, property2);
    EXPECT_EQ(4, property3);
    EXPECT_EQ(4, property4);

    // changing any property affects all
    property1 = 5;
    EXPECT_EQ(5, property1);
    EXPECT_EQ(5, property2);
    EXPECT_EQ(5, property3);
    EXPECT_EQ(5, property4);
    property2 = 10;
    EXPECT_EQ(10, property1);
    EXPECT_EQ(10, property2);
    EXPECT_EQ(10, property3);
    EXPECT_EQ(10, property4);
    property3 = 11;
    EXPECT_EQ(11, property1);
    EXPECT_EQ(11, property2);
    EXPECT_EQ(11, property3);
    EXPECT_EQ(11, property4);
    property4 = 12;
    EXPECT_EQ(12, property1);
    EXPECT_EQ(12, property2);
    EXPECT_EQ(12, property3);
    EXPECT_EQ(12, property4);
}

TEST_F(MetakernelProperties, test_disabled_binding)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);
    EXPECT_EQ(1, property1);
    EXPECT_EQ(2, property2);
    auto binding = metakernel::bindProperties(property1, property2);
    EXPECT_EQ(2, property1);
    EXPECT_EQ(2, property2);

    // change one of the properties at a time
    auto p1c = int(0);
    auto p2c = int(0);
    auto onP1Changed = [&p1c]() { ++p1c; };
    auto onP2Changed = [&p2c]() { ++p2c; };
    property1.changed.connect(onP1Changed);
    property2.changed.connect(onP2Changed);

    property1 = 100;
    EXPECT_EQ(100, property1);
    EXPECT_EQ(100, property2);
    EXPECT_EQ(1, p1c);
    EXPECT_EQ(1, p2c);

    binding->setEnabled(false);
    property2 = 20;
    EXPECT_EQ(100, property1);
    EXPECT_EQ(20, property2);
    EXPECT_EQ(1, p1c);
    EXPECT_EQ(2, p2c);
}

TEST_F(MetakernelProperties, test_property_in_property_binding_destroyed)
{
    metakernel::Lockable host;
    auto property = metakernel::Property<int>(host, 1);
    auto dynamic = new metakernel::Property<int>(host, -1);

    auto binding = property.bind(*dynamic);
    EXPECT_EQ(-1, property);
    EXPECT_EQ(-1, *dynamic);

    *dynamic = 10;
    EXPECT_EQ(10, property);
    EXPECT_EQ(10, *dynamic);

    delete dynamic;
    property = 101;
    EXPECT_FALSE(binding->isAttached());
}

TEST_F(MetakernelProperties, test_stacked_binding)
{
    metakernel::Lockable host;
    metakernel::Property<int> property1(host, 1);
    metakernel::Property<int> property2(host, 2);
    metakernel::Property<int> property3(host, 3);
    metakernel::Property<int> property4(host, 4);

    // All bindings detach on write. Only the active one shall detach!
    auto b1 = property1.bind(property2);
    auto b2 = property1.bind(property3);
    auto b3 = property1.bind(property4);
    EXPECT_FALSE(b1->isEnabled());
    EXPECT_FALSE(b2->isEnabled());
    EXPECT_TRUE(b3->isEnabled());
    EXPECT_EQ(4, property1);

    // modify a property whos binding is disabled.
    property3 = 0;
    EXPECT_EQ(4, property1);
    property4 = 10;
    EXPECT_EQ(10, property1);

    // This removes all the bindings!
    property1 = 1;
    EXPECT_FALSE(b3->isAttached());
    EXPECT_FALSE(b2->isAttached());
    EXPECT_FALSE(b1->isAttached());
    property4 = 9;
    EXPECT_EQ(1, property1);
    property3 = 9;
    EXPECT_EQ(1, property1);
    property2 = 9;
    EXPECT_EQ(1, property1);
}

TEST_F(MetakernelProperties, test_expression_binding)
{
    metakernel::Lockable host;
    metakernel::Property<int> source(host, 10);
    metakernel::Property<std::string> target(host);

    target.bind([&source]()
    {
        return std::to_string(int(source));
    });
    EXPECT_EQ("10"s, std::string(target));

    // update source
    source = 7;
    EXPECT_EQ("7"s, std::string(target));
}

TEST_F(MetakernelProperties, test_bind_to_status)
{
    metakernel::Lockable host;
    metakernel::Property<int> target(host, 1);
    test_property::TestStatus<int> source(host, 10);

    auto binding = target.bind(source);
    EXPECT_EQ(10, target);

    source.setData(99);
    EXPECT_EQ(99, target);
}

TEST_F(MetakernelProperties, test_expression_binding_with_status)
{
    metakernel::Lockable host;
    test_property::TestStatus<int> status(host, 10);
    metakernel::Property<std::string> target(host);

    target.bind([&status]() { return std::to_string(int(status)); });
    EXPECT_EQ("10"s, std::string(target));

    status.setData(99);
    EXPECT_EQ("99"s, std::string(target));
}

TEST_F(MetakernelProperties, test_binding_loop_with_expressions)
{
    metakernel::Lockable host;
    metakernel::Property<int> p1(host, 1);
    metakernel::Property<int> p2(host, 2);
    metakernel::Property<int> p3(host, 3);

    auto b1 = p1.bind([&p2]() { return p2 + 2; });
    EXPECT_EQ(4, p1);
    EXPECT_EQ(2, p2);
    EXPECT_EQ(3, p3);

    auto b2 = p3.bind([&p1]() { return p1 + 4; });
    EXPECT_EQ(4, p1);
    EXPECT_EQ(2, p2);
    EXPECT_EQ(8, p3);

    // This shall cause binding loop
    auto b3 = p2.bind([&p3]() { return p3 + 1; });
    EXPECT_EQ(11, p1);
    EXPECT_EQ(9, p2);
    EXPECT_EQ(15, p3);
}
