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
#include <mox/binding/binding.hpp>
#include <mox/object.hpp>

using namespace mox;

class WritableTest : public Object
{
public:
    static inline PropertyTypeDecl<WritableTest, int, PropertyAccess::ReadWrite> WritableProperty{"writable"};
    PropertyDecl<int> writable{*this, WritableProperty, 0};

    explicit WritableTest() = default;
};

class ReadableTest : public Object
{
public:
    class ROValueProvider : public DefaultValueProvider<int, ValueProviderFlags::Exclusive>
    {
        using Base = DefaultValueProvider<int, ValueProviderFlags::Exclusive>;
    public:
        explicit ROValueProvider()
            : Base(99)
        {
        }

        void setLocalValue(int value)
        {
            update(Variant(value));
        }

    };
    static inline PropertyTypeDecl<ReadableTest, int, PropertyAccess::ReadOnly> ReadableProperty{"readable"};

    std::shared_ptr<ROValueProvider> vpReadable = make_polymorphic_shared<PropertyValueProvider, ROValueProvider>();
    PropertyDecl<int> readable{*this, ReadableProperty, vpReadable};

    explicit ReadableTest() = default;
};


class Bindings : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
    }
};

TEST_F(Bindings, test_one_way_property_binding)
{
    WritableTest o1;
    ReadableTest o2;

    int o1ChangeCount = 0;
    auto onO1Changed = [&o1ChangeCount]() { ++o1ChangeCount; };
    o1.writable.changed.connect(onO1Changed);
    int o2ChangeCount = 0;
    auto onO2Changed = [&o2ChangeCount]() { ++o2ChangeCount; };
    o2.readable.changed.connect(onO2Changed);

    // Bind o1.intProperty with o2.roProperty. This will result in a one-way binding.
    auto binding = Binding::create(o2.readable, o1.writable);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(99, o1.writable);
    EXPECT_EQ(1, o1ChangeCount);
    EXPECT_EQ(0, o2ChangeCount);

    // Update o2 property. This will change o1
    o2.vpReadable->setLocalValue(101);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(2, o1ChangeCount);
    EXPECT_EQ(1, o2ChangeCount);
}

TEST_F(Bindings, test_two_way_property_binding)
{
    WritableTest o1;
    WritableTest o2;

    o1.writable = 10;
    o2.writable = 20;

    int o1ChangeCount = 0;
    auto onO1Changed = [&o1ChangeCount]() { ++o1ChangeCount; };
    o1.writable.changed.connect(onO1Changed);
    int o2ChangeCount = 0;
    auto onO2Changed = [&o2ChangeCount]() { ++o2ChangeCount; };
    o2.writable.changed.connect(onO2Changed);

    auto bindO2ToO1 = Binding::create(o2.writable, o1.writable);
    EXPECT_TRUE(bindO2ToO1->isAttached());
    // as o2 is the source of the binding, the o1 is changed only.
    EXPECT_EQ(1, o1ChangeCount);
    EXPECT_EQ(0, o2ChangeCount);

    WritableTest o3;
    WritableTest o4;
    o3.writable = 30;
    o4.writable = 40;

    // Bind o3 to o1.
    Binding::create(o3.writable, o1.writable);
    // Both o1 and o2 is synced to the value of o3.
    EXPECT_EQ(30, o1.writable);
    EXPECT_EQ(30, o2.writable);
    EXPECT_EQ(2, o1ChangeCount);
    EXPECT_EQ(1, o2ChangeCount);
    // Bind o4 to o1. As this binding disables the previous binding on o2,
    // o2 will not sync o1 anymore.
    Binding::create(o4.writable, o2.writable);
    EXPECT_EQ(30, o1.writable);
    EXPECT_EQ(40, o2.writable);
    EXPECT_EQ(30, o3.writable);
    EXPECT_EQ(2, o1ChangeCount);
    EXPECT_EQ(2, o2ChangeCount);
}

TEST_F(Bindings, test_bing_three_properties_one_way_fail)
{
    WritableTest o1;
    ReadableTest o2;
    ReadableTest o3;

    auto binding = Binding::create(o1.writable, o2.readable, o3.readable);
    EXPECT_NULL(binding);
}

TEST_F(Bindings, test_bing_three_properties_one_way_pass)
{
    WritableTest o1;
    ReadableTest o2;
    WritableTest o3;

    o3.writable = 9;

    auto binding = Binding::create(o1.writable, o2.readable, o3.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(99, o1.writable);
    EXPECT_EQ(99, o2.readable);
    EXPECT_EQ(99, o3.writable);

    o2.vpReadable->setLocalValue(1000);
    EXPECT_EQ(1000, o1.writable);
    EXPECT_EQ(1000, o2.readable);
    EXPECT_EQ(1000, o3.writable);

    // Break the bindings by writing to o1. o3 keeps the value.
    o1.writable = 1;
    EXPECT_EQ(1, o1.writable);
    EXPECT_EQ(1000, o2.readable);
    EXPECT_EQ(1000, o3.writable);

    // Updating o2 won't change o1 and o3 values.
    o2.vpReadable->setLocalValue(8);
    EXPECT_EQ(1, o1.writable);
    EXPECT_EQ(8, o2.readable);
    EXPECT_EQ(1000, o3.writable);
}

TEST_F(Bindings, test_bing_three_properties_two_way)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3;

    o1.writable = 101;

    auto binding = Binding::create(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);
}

TEST_F(Bindings, test_generic_property_binding_dies_on_write)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3;

    o1.writable = 101;

    auto binding = Binding::create(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);

    o3.writable = 5;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(5, o3.writable);
}

TEST_F(Bindings, test_keeponwrite_property_binding_survives_write)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3;

    o1.writable = 101;

    auto binding = Binding::create<ValueProviderFlags::KeepOnWrite>(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);

    o3.writable = 5;
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(5, o1.writable);
    EXPECT_EQ(5, o2.writable);
    EXPECT_EQ(5, o3.writable);

    ++o2.writable;
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(6, o1.writable);
    EXPECT_EQ(6, o2.writable);
    EXPECT_EQ(6, o3.writable);

    --o1.writable;
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(5, o1.writable);
    EXPECT_EQ(5, o2.writable);
    EXPECT_EQ(5, o3.writable);
}

TEST_F(Bindings, test_assign_property_present_in_binding_breaks_binding)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3;

    o1.writable = 101;

    auto binding = Binding::create(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);

    o1.writable = o3.writable;
    EXPECT_FALSE(binding->isAttached());
}
