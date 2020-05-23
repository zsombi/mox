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

class CustomDP : public metakernel::PropertyCore::Data
{
    int data = -1;

public:
    CustomDP(metakernel::PropertyType access = metakernel::PropertyType::ReadWrite)
        : Data(access)
    {
    }
    metakernel::ArgumentData get() const override
    {
        CTRACE(propertyTest, "Custom property data provider getter");
        return data;
    }
    void set(const metakernel::ArgumentData& data) override
    {
        CTRACE(propertyTest, "Custom property data provider setter");
        this->data = data;
    }
    bool isEqual(const metakernel::ArgumentData& other) override
    {
        return data == int(other);
    }
};

}

TEST_F(MetakernelProperties, test_property_api)
{
    metakernel::Property<int> property;
    EXPECT_EQ(0, property);
    EXPECT_EQ(metakernel::PropertyType::ReadWrite, property.getType());

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    property.changed.connect(onPropertyChanged);
    EXPECT_TRACE(propertyTest, "Property value changed to 10");
    property = 10;
    EXPECT_EQ(10, property);
}

TEST_F(MetakernelProperties, test_custom_data_provider)
{
    EXPECT_TRACE(propertyTest, "Custom property data provider getter");
    EXPECT_TRACE(propertyTest, "Custom property data provider setter");
    EXPECT_TRACE(propertyTest, "Property value changed to 10");
    test_property::CustomDP datadProvider;
    metakernel::Property<int> property(datadProvider);

    EXPECT_EQ(metakernel::PropertyType::ReadWrite, property.getType());

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    property.changed.connect(onPropertyChanged);
    property = 10;
    EXPECT_EQ(10, property);
}

TEST_F(MetakernelProperties, test_status_property)
{
    GTEST_SKIP();
    EXPECT_TRACE(propertyTest, "Custom property data provider getter");
    EXPECT_TRACE(propertyTest, "Custom property data provider setter");
    EXPECT_TRACE(propertyTest, "Property value changed to 10");
    test_property::CustomDP datadProvider(metakernel::PropertyType::ReadOnly);
    metakernel::StatusProperty<int> property(datadProvider);
    EXPECT_EQ(metakernel::PropertyType::ReadOnly, property.getType());

    auto onPropertyChanged = [](int value)
    {
        CTRACE(propertyTest, "Property value changed to" << value);
    };
    property.changed.connect(onPropertyChanged);
    EXPECT_EQ(-1, property);
}


TEST_F(MetakernelProperties, test_one_way_binding_discard_on_write)
{
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);

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

TEST_F(MetakernelProperties, test_one_way_binding_keep_on_write)
{
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);

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
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);

    metakernel::BindingGroup::create()->addToGroup(*property1.bind(property2)).addToGroup(*property2.bind(property1)).setPolicy(metakernel::BindingPolicy::DetachOnWrite);
    property1 = 3;
}

TEST_F(MetakernelProperties, test_two_way_binding_of_2_properties_grouped)
{
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);
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
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);
    metakernel::Property<int> property3(3);

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
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);
    metakernel::Property<int> property3(3);
    metakernel::Property<int> property4(4);

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
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);
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
    auto property = metakernel::Property<int>(1);
    auto dynamic = new metakernel::Property<int>(-1);

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
    metakernel::Property<int> property1(1);
    metakernel::Property<int> property2(2);
    metakernel::Property<int> property3(3);
    metakernel::Property<int> property4(4);

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
    metakernel::Property<int> source(10);
    metakernel::Property<std::string> target;

    auto expression = [&source]()
    {
        return std::to_string(int(source));
    };
    target.bind(expression);
    EXPECT_EQ("10"s, std::string(target));

    // update source
    source = 7;
    EXPECT_EQ("7"s, std::string(target));
}
