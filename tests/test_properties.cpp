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

template <typename ValueType>
class TestValueProvider : public PropertyValueProvider
{
    ValueType m_value;

public:
    explicit TestValueProvider(ValueType value, ValueProviderFlags flags = ValueProviderFlags::Generic)
        : PropertyValueProvider(flags)
        , m_value(value)
    {
    }

protected:
    void onActivating() override
    {
        update(Variant(m_value));
    }
};

class PropertyTest : public ObjectLock
{
    class StatusVP : public DefaultValueProvider<bool>
    {
        using Base = DefaultValueProvider<bool>;
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

    class XStatusVP : public DefaultValueProvider<bool, ValueProviderFlags::Exclusive>
    {
        using Base = DefaultValueProvider<bool, ValueProviderFlags::Exclusive>;
    public:
        explicit XStatusVP()
            : Base(true)
        {
        }

        void evaluate(int value)
        {
            update(Variant((value %3) != 0));
        }
    };

    class DriverX : public DefaultValueProvider<int, ValueProviderFlags::Exclusive>
    {
        using Base = DefaultValueProvider<int, ValueProviderFlags::Exclusive>;

    public:
        explicit DriverX()
            : Base(0)
        {
        }

        void setLocalValue(int value)
        {
            update(Variant(int(value / 2)));
        }
    };

public:

    static inline PropertyTypeDecl<PropertyTest, bool, PropertyAccess::ReadWrite> BoolPropertyType{"boolValue"};
    static inline PropertyTypeDecl<PropertyTest, bool, PropertyAccess::ReadOnly> ReadOnlyBoolPropertyType{"status"};
    static inline PropertyTypeDecl<PropertyTest, int, PropertyAccess::ReadWrite> StateChangedPropertyType{"driver"};
    static inline PropertyTypeDecl<PropertyTest, bool, PropertyAccess::ReadOnly> XReadOnlyBoolPropertyType{"statusX"};
    static inline PropertyTypeDecl<PropertyTest, int, PropertyAccess::ReadWrite> XStateChangedPropertyType{"driverX"};

    PropertyDecl<bool> boolValue{*this, BoolPropertyType, true};
    PropertyDecl<bool> status{*this, ReadOnlyBoolPropertyType, std::make_shared<StatusVP>()};
    PropertyDecl<int> driver{*this, StateChangedPropertyType, 0};
    PropertyDecl<bool> statusX{*this, XReadOnlyBoolPropertyType, std::make_shared<XStatusVP>()};
    PropertyDecl<int> driverX{*this, XStateChangedPropertyType, std::make_shared<DriverX>()};

    explicit PropertyTest()
    {
        StatusVP* vp = dynamic_cast<StatusVP*>(status.getDefaultValueProvider().get());
        EXPECT_NOT_NULL(vp);
        auto conn = driver.changed.connect(*vp, &StatusVP::evaluate);
        EXPECT_NOT_NULL(conn);

        XStatusVP* xvp = dynamic_cast<XStatusVP*>(statusX.getDefaultValueProvider().get());
        EXPECT_NOT_NULL(xvp);
        conn = driverX.changed.connect(*xvp, &XStatusVP::evaluate);
        EXPECT_NOT_NULL(conn);

        DriverX* x = dynamic_cast<DriverX*>(driverX.getExclusiveValueProvider().get());
        EXPECT_NOT_NULL(x);
        driver.changed.connect(*x, &DriverX::setLocalValue);
    }
};

class PropertyMetatypeTest : public Object
{
    PropertyValueProviderSharedPtr enabler = std::make_shared<DefaultValueProvider<bool>>(true);

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

    PropertyDecl<int> intValue{*this, StaticMetaClass::IntPropertyType, -1};
    PropertyDecl<bool> enabled{*this, StaticMetaClass::ReadOnlyBoolPropertyType, enabler};
    PropertyDecl<std::string> stringValue{*this, StaticMetaClass::StringPropertyType, "alpha"};
};

class CustomDefaultValueProvider : public PropertyValueProvider
{
    int defaultValue;

    Variant getLocalValue() const override
    {
        return Variant(defaultValue);
    }

public:
    explicit CustomDefaultValueProvider(int defaultValue)
        : PropertyValueProvider(ValueProviderFlags::Default)
        , defaultValue(defaultValue)
    {}

    static auto create(int defaultValue)
    {
        return make_polymorphic_shared<PropertyValueProvider, CustomDefaultValueProvider>(defaultValue);
    }
};


template <typename ValueType>
class ExclusiveVP : public DefaultValueProvider<ValueType, ValueProviderFlags::Exclusive>
{
    using Base = DefaultValueProvider<ValueType, ValueProviderFlags::Exclusive>;

    explicit ExclusiveVP(ValueType defValue)
        : Base(defValue)
    {
    }

public:

    static std::shared_ptr<ExclusiveVP<ValueType>> create(ValueType defValue)
    {
        return make_polymorphic_shared_ptr<PropertyValueProvider>(new ExclusiveVP<ValueType>(defValue));
    }

    void setLocalValue(ValueType value)
    {
        this->update(Variant(value));
    }
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

    EXPECT_THROW(test.status = false, mox::Exception);
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

TEST_F(Properties, test_add_new_default_value_provider)
{
    PropertyTest test;

    EXPECT_EQ(0, test.driver);

    auto vp1 = CustomDefaultValueProvider::create(1010);
    EXPECT_FALSE(vp1->isAttached());

    EXPECT_THROW(vp1->attach(test.driver), mox::Exception);
}

TEST_F(Properties, test_add_new_value_provider)
{
    PropertyTest test;

    EXPECT_EQ(0, test.driver);

    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    EXPECT_FALSE(vp1->isAttached());
    EXPECT_NO_THROW(vp1->attach(test.driver));
    EXPECT_TRUE(vp1->isAttached());
}

TEST_F(Properties, test_attach_again)
{
    PropertyTest test;
    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    EXPECT_NO_THROW(vp1->attach(test.driver));
    EXPECT_THROW(vp1->attach(test.driver), mox::Exception);
}

TEST_F(Properties, test_detach_again)
{
    PropertyTest test;
    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    EXPECT_NO_THROW(vp1->attach(test.driver));
    EXPECT_NO_THROW(vp1->detach());
    EXPECT_THROW(vp1->detach(), mox::Exception);
}

TEST_F(Properties, test_atach_to_two_properties)
{
    PropertyTest test1;
    PropertyMetatypeTest test2;

    auto vp = std::make_shared<TestValueProvider<int>>(1010);
    vp->attach(test1.driver);

    EXPECT_TRUE(vp->isAttached());
    EXPECT_THROW(vp->attach(test2.intValue), mox::Exception);
}

TEST_F(Properties, test_remove_value_provider)
{
    PropertyTest test;
    int triggerCount = 0;
    auto onDriverChanged = [&triggerCount]()
    {
        ++triggerCount;
    };
    test.driver.changed.connect(onDriverChanged);

    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    vp1->attach(test.driver);
    auto vp2 = std::make_shared<TestValueProvider<int>>(2030);
    vp2->attach(test.driver);

    EXPECT_EQ(2, triggerCount);
    EXPECT_EQ(2030, test.driver);

    // remove vp1
    EXPECT_TRUE(vp1->isAttached());
    vp1->detach();
    EXPECT_EQ(2, triggerCount);
    EXPECT_EQ(2030, test.driver);
}

TEST_F(Properties, test_set_property_value_detaches_value_providers)
{
    PropertyTest test;
    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    vp1->attach(test.driver);
    auto vp2 = std::make_shared<TestValueProvider<int>>(2030);
    vp2->attach(test.driver);
    EXPECT_EQ(2030, test.driver);

    int triggerCount = 0;
    auto onDriverChanged = [&triggerCount]()
    {
        ++triggerCount;
    };
    test.driver.changed.connect(onDriverChanged);

    test.driver = 10;
    EXPECT_EQ(1, triggerCount);
    EXPECT_FALSE(vp1->isAttached());
    EXPECT_FALSE(vp2->isAttached());
}

TEST_F(Properties, test_attach_exclusive_value_providers)
{
    PropertyTest test;

    auto defaultVP = test.driver.getDefaultValueProvider();
    EXPECT_TRUE(defaultVP->isAttached());

    auto vp1 = ExclusiveVP<int>::create(1234);
    EXPECT_NO_THROW(vp1->attach(test.driver));
    EXPECT_TRUE(vp1->isAttached());
    EXPECT_TRUE(vp1->isEnabled());
    EXPECT_EQ(1234, test.driver);

    // The second exclusive detaches the previous
    auto vp2 = ExclusiveVP<int>::create(999);
    EXPECT_THROW(vp2->attach(test.driver), Exception);
    EXPECT_FALSE(vp2->isAttached());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_TRUE(vp1->isAttached());
    EXPECT_TRUE(vp1->isEnabled());
    EXPECT_EQ(1234, test.driver);

    // Write to the property.
    test.driver = 1;
    EXPECT_EQ(1234, test.driver);
}

TEST_F(Properties, test_write_to_property_with_exclusive_default_value_provider)
{
    PropertyTest test;

    test.driverX = 91234;
    EXPECT_NE(91234, test.driverX);
}

TEST_F(Properties, test_attach_exclusive_value_provider_to_property_with_default_exclusive_value_provider)
{
    PropertyTest test;

    auto vp = ExclusiveVP<int>::create(1234);
    EXPECT_THROW(vp->attach(test.driverX), Exception);
    EXPECT_NE(1234, test.driverX);
}

TEST_F(Properties, test_attach_normal_value_provider_after_exclusive)
{
    PropertyTest test;
    // set the value to differ from the default value before other value providers are attached.
    test.driver = 3;
    EXPECT_EQ(3, test.driver);

    auto vp1 = std::make_shared<DefaultValueProvider<int, ValueProviderFlags::Exclusive>>(9030);
    vp1->attach(test.driver);
    EXPECT_EQ(9030, test.driver);
    EXPECT_TRUE(vp1->isAttached());
    EXPECT_TRUE(vp1->isEnabled());

    auto vp2 = std::make_shared<TestValueProvider<int>>(2030);
    vp2->attach(test.driver);
    EXPECT_EQ(9030, test.driver);
    EXPECT_TRUE(vp1->isAttached());
    EXPECT_TRUE(vp2->isAttached());
    EXPECT_TRUE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
}

TEST_F(Properties, test_reset_property_with_default_value_provider)
{
    PropertyTest test;

    test.driverX.reset();
    EXPECT_NOT_NULL(test.driverX.getDefaultValueProvider());
    EXPECT_NOT_NULL(test.driverX.getExclusiveValueProvider());
}

TEST_F(Properties, test_property_reset_keeps_exclusive_value_providers)
{
    PropertyTest test;

    auto vp = ExclusiveVP<int>::create(1234);
    vp->attach(test.driver);
    EXPECT_EQ(1234, test.driver);
    EXPECT_EQ(vp, test.driver.getExclusiveValueProvider());

    test.driver.reset();
    EXPECT_TRUE(vp->isAttached());
}

TEST_F(Properties, test_property_setter_keeps_keep_on_write_value_providers)
{
    PropertyTest test;

    auto vp = std::make_shared<TestValueProvider<int>>(10, ValueProviderFlags::KeepOnWrite);
    vp->attach(test.driver);
    EXPECT_TRUE(vp->isAttached());

    // set the value for the property; the vp is kept attached
    test.driver = 11;
    EXPECT_TRUE(vp->isAttached());
}

TEST_F(Properties, test_reset_to_default_value_detaches_value_providers)
{
    PropertyTest test;
    // set the value to differ from the default value before other value providers are attached.
    test.driver = 3;
    EXPECT_EQ(3, test.driver);

    auto vp1 = std::make_shared<TestValueProvider<int>>(1010);
    vp1->attach(test.driver);
    auto vp3 = std::make_shared<DefaultValueProvider<int, ValueProviderFlags::Exclusive>>(9030);
    vp3->attach(test.driver);
    auto vp2 = std::make_shared<TestValueProvider<int>>(2030, ValueProviderFlags::KeepOnWrite);
    vp2->attach(test.driver);
    EXPECT_EQ(9030, test.driver);

    int triggerCount = 0;
    auto onDriverChanged = [&triggerCount]()
    {
        ++triggerCount;
    };
    test.driver.changed.connect(onDriverChanged);

    test.driver.reset();
    // Due to exclusive VP, the value does not change ever.
    EXPECT_EQ(0, triggerCount);
    EXPECT_FALSE(vp1->isAttached());
    EXPECT_FALSE(vp2->isAttached());
    EXPECT_TRUE(vp3->isAttached());
    EXPECT_EQ(9030, test.driver);
}

TEST_F(Properties, test_detach_disabled_value_providers)
{
    PropertyTest test;

    auto vp1 = std::make_shared<TestValueProvider<int>>(100);
    auto vp2 = std::make_shared<TestValueProvider<int>>(200);
    auto vp3 = std::make_shared<TestValueProvider<int>>(300);

    vp1->attach(test.driver);
    vp2->attach(test.driver);
    vp3->attach(test.driver);
    EXPECT_TRUE(vp1->isAttached());
    EXPECT_TRUE(vp2->isAttached());
    EXPECT_TRUE(vp3->isAttached());
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_TRUE(vp3->isEnabled());
    EXPECT_EQ(300, test.driver);

    vp1->detach();
    EXPECT_FALSE(vp1->isAttached());
    EXPECT_TRUE(vp2->isAttached());
    EXPECT_TRUE(vp3->isAttached());
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_TRUE(vp3->isEnabled());
    EXPECT_EQ(300, test.driver);

    vp2->detach();
    EXPECT_FALSE(vp1->isAttached());
    EXPECT_FALSE(vp2->isAttached());
    EXPECT_TRUE(vp3->isAttached());
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_TRUE(vp3->isEnabled());
    EXPECT_EQ(300, test.driver);
}

TEST_F(Properties, test_value_providers_enablement)
{
    PropertyTest test;

    auto vp1 = std::make_shared<TestValueProvider<int>>(100);
    auto vp2 = std::make_shared<TestValueProvider<int>>(200);
    auto vp3 = std::make_shared<TestValueProvider<int>>(300);

    EXPECT_EQ(0, test.driver);
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_FALSE(vp3->isEnabled());

    vp1->attach(test.driver);
    EXPECT_TRUE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_FALSE(vp3->isEnabled());
    EXPECT_EQ(100, test.driver);

    vp2->attach(test.driver);
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_TRUE(vp2->isEnabled());
    EXPECT_FALSE(vp3->isEnabled());
    EXPECT_EQ(200, test.driver);

    vp3->attach(test.driver);
    EXPECT_FALSE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_TRUE(vp3->isEnabled());
    EXPECT_EQ(300, test.driver);

    // enable vp1
    vp1->setEnabled(true);
    EXPECT_TRUE(vp1->isEnabled());
    EXPECT_FALSE(vp2->isEnabled());
    EXPECT_FALSE(vp3->isEnabled());
    // The value stays as it was before vp1 got enabled, as vp1 doesn't change the value when enabled.
    EXPECT_EQ(300, test.driver);
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

TEST_F(Properties, test_metaproperty_set_detaches_value_providers)
{
    PropertyMetatypeTest test;

    auto vp1 = std::make_shared<TestValueProvider<int>>(123);
    vp1->attach(test.intValue);

    EXPECT_EQ(123, test.intValue);
    EXPECT_TRUE(vp1->isAttached());

    EXPECT_TRUE(setProperty(test, "intValue", 321));
    EXPECT_FALSE(vp1->isAttached());
}
