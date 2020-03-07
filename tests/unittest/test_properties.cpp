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
#include <mox/meta/property/property.hpp>

using namespace mox;

class PropertyTest : public MetaBase
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
            update(Variant((value %3) != 0));
        }
    };

    StatusVP statusVP;

    PropertyData<bool> boolValueData{true};
    PropertyData<int> driverData{0};

public:

    static inline SignalTypeDecl<bool> BoolChangedSignalType;
    static inline SignalTypeDecl<bool> ReadOnlyChangedSignalType;
    static inline SignalTypeDecl<int> StateChangedSignalType;
    static inline PropertyTypeDecl<bool, PropertyAccess::ReadWrite> BoolPropertyType = {BoolChangedSignalType, true};
    static inline PropertyTypeDecl<bool, PropertyAccess::ReadOnly> ReadOnlyBoolPropertyType = {ReadOnlyChangedSignalType, true};
    static inline PropertyTypeDecl<int, PropertyAccess::ReadWrite> StateChangedPropertyType = {StateChangedSignalType};

    Property boolValue{*this, BoolPropertyType, boolValueData};
    Property status{*this, ReadOnlyBoolPropertyType, statusVP};
    Property driver{*this, StateChangedPropertyType, driverData};

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
    PropertyData<int> intValueData{-1};
    PropertyData<std::string> stringValueData{"alpha"s};

public:

    static std::shared_ptr<PropertyMetatypeTest> create(Object* parent = nullptr)
    {
        return createObject<PropertyMetatypeTest>(new PropertyMetatypeTest, parent);
    }

    MetaInfo(PropertyMetatypeTest, Object)
    {
        static inline MetaSignal<PropertyMetatypeTest, int> IntChangedSignalType{"intValueChanged"};
        static inline MetaSignal<PropertyMetatypeTest, bool> ReadOnlyBoolChangedSignalType{"enabledChanged"};
        static inline MetaSignal<PropertyMetatypeTest, int> StringChangedSignalType{"stringValueChanged"};
        static inline MetaProperty<PropertyMetatypeTest, int, PropertyAccess::ReadWrite> IntPropertyType{IntChangedSignalType, "intValue", -1};
        static inline MetaProperty<PropertyMetatypeTest, bool, PropertyAccess::ReadOnly> ReadOnlyBoolPropertyType{ReadOnlyBoolChangedSignalType, "enabled", true};
        static inline MetaProperty<PropertyMetatypeTest, std::string, PropertyAccess::ReadWrite> StringPropertyType{StringChangedSignalType, "stringValue", "alpha"s};
    };

    Property intValue{*this, StaticMetaClass::IntPropertyType, intValueData};
    Property enabled{*this, StaticMetaClass::ReadOnlyBoolPropertyType, selfEnabler};
    Property stringValue{*this, StaticMetaClass::StringPropertyType, stringValueData};
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

TEST_F(Properties, test_property_types_used)
{
    EXPECT_EQ(Metatype::String, Object::StaticMetaClass::ObjectNameProperty.getValueType().getType());
    EXPECT_EQ(Metatype::Bool, PropertyTest::BoolPropertyType.getValueType().getType());
    EXPECT_EQ(Metatype::Int32, PropertyTest::StateChangedPropertyType.getValueType().getType());
}

TEST_F(Properties, test_properties_no_metatype)
{
    PropertyTest test;

    EXPECT_TRUE(bool(test.boolValue));
    EXPECT_TRUE(bool(test.status));
    EXPECT_FALSE(test.boolValue.isReadOnly());
    EXPECT_TRUE(test.status.isReadOnly());
}

TEST_F(Properties, test_properties_is_metatype)
{
    PropertyMetatypeTest test;

    EXPECT_TRUE(bool(test.enabled));
    EXPECT_EQ(-1, int(test.intValue));
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
    EXPECT_TRUE(bool(test.boolValue));

    test.boolValue = false;
    EXPECT_TRUE(signaled);
    EXPECT_FALSE(bool(test.boolValue));
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

    EXPECT_TRUE(bool(test.status));
    EXPECT_EQ(0, int(test.driver));

    test.driver = 3;
    EXPECT_FALSE(bool(test.status));
    EXPECT_TRUE(statusChanged);
    EXPECT_EQ(3, int(test.driver));
}

TEST_F(Properties, test_reset_to_default_value)
{
    PropertyTest test;

    EXPECT_EQ(0, int(test.driver));
    test.driver = 132;
    EXPECT_EQ(132, int(test.driver));

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

    EXPECT_EQ(-1, int(test.getProperty(PropertyMetatypeTest::StaticMetaClass::IntPropertyType)));
    EXPECT_EQ(true, bool(test.getProperty(PropertyMetatypeTest::StaticMetaClass::ReadOnlyBoolPropertyType)));
    EXPECT_EQ("alpha"s, std::string(test.getProperty(PropertyMetatypeTest::StaticMetaClass::StringPropertyType)));
}

TEST_F(Properties, test_metaproperty_get)
{
    PropertyMetatypeTest test;
    auto mc = PropertyMetatypeTest::StaticMetaClass::get();
    test.objectName = "testObject"s;

    EXPECT_EQ(-1, int(test.getProperty(mc->IntPropertyType)));
    EXPECT_EQ(true, bool(test.getProperty(mc->ReadOnlyBoolPropertyType)));
    EXPECT_EQ("alpha"s, std::string(test.getProperty(mc->StringPropertyType)));

    EXPECT_EQ(-1, *metainfo::getProperty<int>(test, "intValue"));
    EXPECT_EQ(true, *metainfo::getProperty<bool>(test, "enabled"));
    EXPECT_EQ("alpha"s, *metainfo::getProperty<std::string>(test, "stringValue"));
    EXPECT_EQ(std::nullopt, metainfo::getProperty<int>(test, "IntValue"));
    EXPECT_EQ("testObject"s, *metainfo::getProperty<std::string>(test, "objectName"));
}

TEST_F(Properties, test_metaproperty_set)
{
    PropertyMetatypeTest test;
    auto mc = PropertyMetatypeTest::StaticMetaClass::get();

    EXPECT_NOT_NULL(test.setProperty(mc->IntPropertyType, Variant(2)));
    EXPECT_THROW(test.setProperty(mc->ReadOnlyBoolPropertyType, Variant(true)), mox::Exception);
    EXPECT_NOT_NULL(test.setProperty(mc->StringPropertyType, Variant("stew"s)));

    EXPECT_TRUE(metainfo::setProperty(test, "intValue", 20));
    EXPECT_THROW(metainfo::setProperty(test, "enabled", true), mox::Exception);
    EXPECT_TRUE(metainfo::setProperty(test, "stringValue", "stake"s));
    EXPECT_FALSE(metainfo::setProperty(test, "IntValue", 21));
}

static SignalTypeDecl<int> StandaloneIntSignalType;
static PropertyTypeDecl<int, PropertyAccess::ReadWrite> StandaloneIntPropertyType = {StandaloneIntSignalType};

TEST_F(Properties, test_property_added_runtime)
{
    PropertyTest test;
    PropertyData<int> dynamicData1{-1};

    Property runtimeInt(test, StandaloneIntPropertyType, dynamicData1);

    EXPECT_EQ(-1, int(runtimeInt));

    int callCount = 0;
    auto slot = [&callCount]() { ++callCount; };
    runtimeInt.changed.connect(slot);

    runtimeInt = 101;
    EXPECT_EQ(1, callCount);
    EXPECT_EQ(101, int(runtimeInt));
}

TEST_F(Properties, test_dynamic_property)
{
    auto test = std::make_shared<PropertyTest>();
    PropertyData<int> dataProvider{1};

    auto runtime = test->setProperty(StandaloneIntPropertyType, Variant(1));
    EXPECT_NOT_NULL(dynamic_cast<DynamicProperty*>(runtime));
    DynamicPropertyWeak runtimeInt = dynamic_cast<DynamicProperty*>(runtime)->shared_from_this();

    EXPECT_EQ(1, int(*runtime));
    EXPECT_TRUE(runtime->isValid());

    test.reset();
    EXPECT_TRUE(runtimeInt.expired());
}

TEST_F(Properties, test_dynamic_property_pointer_held_after_host_deletion)
{
    auto test = std::make_shared<PropertyTest>();
    PropertyData<int> dataProvider{1};

    auto runtime = test->setProperty(StandaloneIntPropertyType, Variant(1));
    EXPECT_NOT_NULL(dynamic_cast<DynamicProperty*>(runtime));
    DynamicPropertyPtr runtimeInt = dynamic_cast<DynamicProperty*>(runtime)->shared_from_this();

    EXPECT_EQ(1, int(*runtime));
    EXPECT_TRUE(runtime->isValid());

    test.reset();
    EXPECT_NOT_NULL(runtimeInt);
    EXPECT_FALSE(runtimeInt->isValid());
}
