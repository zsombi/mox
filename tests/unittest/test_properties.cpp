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
#include <mox/config/deftypes.hpp>
#include <mox/utils/locks.hpp>
#include <mox/property/property.hpp>

using namespace mox;

class PropertyTest : public ObjectLock
{
    class StatusVP : public PropertyData<bool>
    {
        using Base = PropertyData<bool>;

    public:
        explicit StatusVP()
            : Base(true)
        {
        }

        void evaluate(int value)
        {
            updateData(Variant((value %3) != 0));
        }
    };

    StatusVP statusVP;

public:

    static inline PropertyTypeDecl<PropertyTest, bool, PropertyAccess::ReadWrite> BoolPropertyType{"boolValue"};
    static inline PropertyTypeDecl<PropertyTest, bool, PropertyAccess::ReadOnly> ReadOnlyBoolPropertyType{"status"};
    static inline PropertyTypeDecl<PropertyTest, int, PropertyAccess::ReadWrite> StateChangedPropertyType{"driver"};

    WritableProperty<bool> boolValue{*this, BoolPropertyType, true};
    ReadOnlyProperty<bool> status{*this, ReadOnlyBoolPropertyType, statusVP};
    WritableProperty<int> driver{*this, StateChangedPropertyType, 0};

    explicit PropertyTest()
    {
        auto conn = driver.changed.connect(statusVP, &StatusVP::evaluate);
        EXPECT_NOT_NULL(conn);
    }
};

class PropertyMetatypeTest : public Object
{
    class Enabler : public PropertyData<bool>
    {
        using Base = PropertyData<bool>;

    public:
        static inline Enabler self();
        Enabler()
            : Base(true)
        {
        }
    };

    Enabler selfEnabler;

public:

    static std::shared_ptr<PropertyMetatypeTest> create(Object* parent = nullptr)
    {
        return createObject<PropertyMetatypeTest>(new PropertyMetatypeTest, parent);
    }

    ClassMetaData(PropertyMetatypeTest, Object)
    {
        static inline PropertyTypeDecl<PropertyMetatypeTest, int, PropertyAccess::ReadWrite> IntPropertyType{"intValue"};
        static inline PropertyTypeDecl<PropertyMetatypeTest, bool, PropertyAccess::ReadOnly> ReadOnlyBoolPropertyType{"enabled"};
        static inline PropertyTypeDecl<PropertyMetatypeTest, std::string, PropertyAccess::ReadWrite> StringPropertyType{"stringValue"};
    };

    WritableProperty<int> intValue{*this, StaticMetaClass::IntPropertyType, -1};
    ReadOnlyProperty<bool> enabled{*this, StaticMetaClass::ReadOnlyBoolPropertyType, selfEnabler};
    WritableProperty<std::string> stringValue{*this, StaticMetaClass::StringPropertyType, "alpha"};
};


class Properties : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();

        registerMetaClass<PropertyMetatypeTest>();
        PropertyMetatypeTest::StaticMetaClass::get();
        Object::StaticMetaClass::get();
        MetaObject::StaticMetaClass::get();
    }
};

TEST_F(Properties, test_property_type)
{
    EXPECT_EQ(Metatype::String, Object::StaticMetaClass::ObjectNameProperty.getValueType().getType());
    EXPECT_EQ(Metatype::Bool, PropertyTest::BoolPropertyType.getValueType().getType());
    EXPECT_EQ(Metatype::Int32, PropertyTest::StateChangedPropertyType.getValueType().getType());
}

TEST_F(Properties, test_properties_no_metatype)
{
    PropertyTest test;

    EXPECT_TRUE(test.boolValue);
    EXPECT_TRUE(test.status);
    EXPECT_FALSE(test.boolValue.isReadOnly());
    EXPECT_TRUE(test.status.isReadOnly());
}

TEST_F(Properties, test_properties_is_metatype)
{
    PropertyMetatypeTest test;

    EXPECT_TRUE(test.enabled);
    EXPECT_EQ(-1, test.intValue);
    std::string str = test.stringValue;
    EXPECT_EQ("alpha"s, str);
}

TEST_F(Properties, test_readonly_property_setter_throws)
{
    PropertyTest test;
    Property* testProperty = &test.status;

    EXPECT_THROW(testProperty->set(Variant(false)), mox::Exception);
}

TEST_F(Properties, test_emit_signal_on_property_change)
{
    PropertyTest test;

    auto signaled = false;
    auto onBoolValueChanged = [&signaled]()
    {
        signaled = true;
    };
    EXPECT_NOT_NULL(test.boolValue.changed.connect(onBoolValueChanged));

    EXPECT_FALSE(signaled);
    EXPECT_TRUE(test.boolValue);

    test.boolValue = false;
    EXPECT_TRUE(signaled);
    EXPECT_FALSE(test.boolValue);
}

TEST_F(Properties, test_drive_readonly_property_through_default_value_provider)
{
    PropertyTest test;
    bool statusChanged = false;
    auto onStatusChanged = [&statusChanged]()
    {
        statusChanged = true;
    };

    EXPECT_NOT_NULL(test.status.changed.connect(onStatusChanged));

    EXPECT_TRUE(test.status);
    EXPECT_EQ(0, test.driver);

    test.driver = 3;
    EXPECT_FALSE(test.status);
    EXPECT_TRUE(statusChanged);
    EXPECT_EQ(3, test.driver);
}

TEST_F(Properties, test_reset_to_default_value)
{
    PropertyTest test;

    EXPECT_EQ(0, test.driver);
    test.driver = 132;
    EXPECT_EQ(132, test.driver);

    bool resetCalled = false;
    auto onReset = [&resetCalled]()
    {
        resetCalled = true;
    };
    test.driver.changed.connect(onReset);
    test.driver.reset();
    EXPECT_TRUE(resetCalled);
}



TEST_F(Properties, test_metaproperty)
{
    PropertyMetatypeTest test;

    EXPECT_EQ(-1, PropertyMetatypeTest::StaticMetaClass::IntPropertyType.get(&test));
    EXPECT_EQ(true, PropertyMetatypeTest::StaticMetaClass::ReadOnlyBoolPropertyType.get(&test));
    EXPECT_EQ("alpha"s, PropertyMetatypeTest::StaticMetaClass::StringPropertyType.get(&test));
}

TEST_F(Properties, test_metaproperty_get)
{
    PropertyMetatypeTest test;
    auto mc = PropertyMetatypeTest::StaticMetaClass::get();
    test.objectName = "testObject";

    EXPECT_EQ(-1, mc->IntPropertyType.get(&test));
    EXPECT_EQ(true, mc->ReadOnlyBoolPropertyType.get(&test));
    EXPECT_EQ("alpha"s, mc->StringPropertyType.get(&test));

    EXPECT_EQ(std::make_pair(-1, true), property<int>(test, "intValue"));
    EXPECT_EQ(std::make_pair(true, true), property<bool>(test, "enabled"));
    EXPECT_EQ(std::make_pair("alpha"s, true), property<std::string>(test, "stringValue"));
    EXPECT_FALSE(property<int>(test, "IntValue").second);
    EXPECT_EQ(std::make_pair("testObject"s, true), property<std::string>(test, "objectName"));
}

TEST_F(Properties, test_metaproperty_set)
{
    PropertyMetatypeTest test;
    auto mc = PropertyMetatypeTest::StaticMetaClass::get();

    EXPECT_TRUE(mc->IntPropertyType.set(&test, Variant(2)));
    EXPECT_THROW(mc->ReadOnlyBoolPropertyType.set(&test, Variant(true)), mox::Exception);
    EXPECT_TRUE(mc->StringPropertyType.set(&test, Variant("stew"s)));

    EXPECT_TRUE(setProperty(test, "intValue", 20));
    EXPECT_THROW(setProperty(test, "enabled", true), mox::Exception);
    EXPECT_TRUE(setProperty(test, "stringValue", "stake"s));
    EXPECT_FALSE(setProperty(test, "IntValue", 21));
}
