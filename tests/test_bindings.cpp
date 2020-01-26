/*
 * Copyright (C) 2017-2020 bitWelder
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
#include <mox/binding/binding_group.hpp>
#include <mox/binding/property_binding.hpp>
#include <mox/binding/expression_binding.hpp>
#include <mox/object.hpp>

using namespace mox;

class WritableTest : public Object
{
public:
    static inline PropertyTypeDecl<WritableTest, int, PropertyAccess::ReadWrite> WritablePropertyType{"writable"};
    WritableProperty<int> writable{*this, WritablePropertyType, 0};

    explicit WritableTest(int initialValue = 0)
    {
        writable = initialValue;
    }
};

class ReadableTest : public Object
{
public:
    PropertyData<int> vpReadable = 99;

    static inline PropertyTypeDecl<ReadableTest, int, PropertyAccess::ReadOnly> ReadablePropertyType{"readable"};
    ReadOnlyProperty<int> readable{*this, ReadablePropertyType, vpReadable};

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

TEST_F(Bindings, test_bind_read_only_properties)
{
    WritableTest o1;
    ReadableTest o2;
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.readable);
    EXPECT_NOT_NULL(binding1);
    auto binding2 = PropertyBinding::bindPermanent(o2.readable, o1.writable);
    EXPECT_NULL(binding2);
    auto binding3 = PropertyBinding::bindPermanent(o3.readable, o2.readable);
    EXPECT_NULL(binding3);
}

TEST_F(Bindings, test_bind_writable_properties)
{
    WritableTest o1;
    WritableTest o2;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding1);
    EXPECT_EQ(&o1.writable, binding1->getTarget());
    EXPECT_EQ(binding1, o1.writable.getCurrentBinding());
    EXPECT_NULL(binding1->getBindingGroup());
    EXPECT_TRUE(binding1->isAttached());
    EXPECT_TRUE(binding1->isEnabled());
    EXPECT_TRUE(binding1->isPermanent());

    auto binding2 = PropertyBinding::bindAutoDiscard(o2.writable, o1.writable);
    EXPECT_NOT_NULL(binding2);
    EXPECT_EQ(&o2.writable, binding2->getTarget());
    EXPECT_EQ(binding2, o2.writable.getCurrentBinding());
    EXPECT_NULL(binding2->getBindingGroup());
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_FALSE(binding2->isPermanent());
}

TEST_F(Bindings, test_permanent_binding_survives_target_write)
{
    WritableTest o1;
    WritableTest o2;

    auto binding = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_TRUE(binding->isPermanent());
    EXPECT_EQ(binding, o1.writable.getCurrentBinding());

    // Write to the target
    o1.writable = 1000;
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(binding, o1.writable.getCurrentBinding());
}

TEST_F(Bindings, test_auto_discard_binding_detached_on_target_write)
{
    WritableTest o1;
    WritableTest o2;

    auto binding = PropertyBinding::bindAutoDiscard(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_FALSE(binding->isPermanent());
    EXPECT_EQ(binding, o1.writable.getCurrentBinding());

    // Write to the target
    o1.writable = 1000;
    EXPECT_FALSE(binding->isAttached());
    EXPECT_NE(binding, o1.writable.getCurrentBinding());
}

TEST_F(Bindings, test_multiple_bindings_on_target)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o3.readable);

    EXPECT_TRUE(binding1->isAttached());
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);
}

TEST_F(Bindings, test_re_enable_binding_on_target_with_enable_to_evaluate)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o3.readable);

    EXPECT_TRUE(binding1->isAttached());
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);

    binding1->setEnabled(true);
    EXPECT_TRUE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(20, o1.writable);

    binding2->setEnabled(true);
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);
}

TEST_F(Bindings, test_re_enable_binding_on_target_with_enable_to_not_evaluate)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    binding1->setEvaluateOnEnabled(false);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o3.readable);
    binding2->setEvaluateOnEnabled(false);

    EXPECT_TRUE(binding1->isAttached());
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);

    binding1->setEnabled(true);
    EXPECT_TRUE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);
}

TEST_F(Bindings, test_disable_all_bindings)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o3.readable);

    EXPECT_EQ(99, o1.writable);

    binding2->setEnabled(false);
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(99, o1.writable);

    // update readable
    o3.vpReadable.updateData(Variant(1000));
    EXPECT_EQ(99, o1.writable);

    o2.writable = 1;
    EXPECT_EQ(99, o1.writable);
}


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
    auto binding = PropertyBinding::bindPermanent(o1.writable, o2.readable);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(&o1.writable, binding->getTarget());
    EXPECT_EQ(99, o1.writable);
    EXPECT_EQ(1, o1ChangeCount);
    EXPECT_EQ(0, o2ChangeCount);

    // Update o2 property. This will change o1
    o2.vpReadable.updateData(Variant(101));
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(2, o1ChangeCount);
    EXPECT_EQ(1, o2ChangeCount);
}

TEST_F(Bindings, test_one_way_property_binding_removed_explicitly)
{
    WritableTest o1;
    ReadableTest o2;

    // Bind o1.intProperty with o2.roProperty. This will result in a one-way binding.
    auto binding = PropertyBinding::bindPermanent(o1.writable, o2.readable);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(99, o1.writable);

    binding->detach();
    EXPECT_FALSE(binding->isAttached());

    // Update o2 property. This will no longer change o1, as the binding is removed.
    o2.vpReadable.updateData(Variant(101));
    EXPECT_EQ(99, o1.writable);
}

TEST_F(Bindings, test_two_way_binding)
{
    WritableTest o1;
    WritableTest o2;

    o1.writable = 10;
    o2.writable = 20;

    auto binding1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    EXPECT_NOT_NULL(binding1);
    EXPECT_TRUE(binding1->isAttached());
    EXPECT_EQ(&o2.writable, binding1->getTarget());
    EXPECT_EQ(10, o2.writable);

    // o2 write does not update o1, only the other way around.
    o1.writable = 5;
    EXPECT_EQ(5, o2.writable);
    o2.writable = 9;
    EXPECT_EQ(5, o1.writable);
    EXPECT_EQ(9, o2.writable);

    // Create binding the other way around
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding2);
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_EQ(&o1.writable, binding2->getTarget());
    EXPECT_EQ(9, o1.writable);

    // Writes to either of the property will update both
    o1.writable = 0;
    EXPECT_EQ(0, o2.writable);
    o2.writable = 99;
    EXPECT_EQ(99, o1.writable);
}

TEST_F(Bindings, test_two_way_binding_removed_explicitly)
{
    WritableTest o1;
    WritableTest o2;

    o1.writable = 10;
    o2.writable = 20;

    auto binding1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding1);
    EXPECT_NOT_NULL(binding2);

    binding1->detach();
    o1.writable = 100;
    EXPECT_EQ(10, o2.writable);
    o2.writable = 80;
    EXPECT_EQ(80, o1.writable);

    binding2->detach();
    o1.writable = 100;
    EXPECT_EQ(80, o2.writable);
    o2.writable = 80;
    EXPECT_EQ(100, o1.writable);
}

TEST_F(Bindings, test_multiple_permanent_bindings_on_target)
{
    WritableTest o1;
    WritableTest o2;
    ReadableTest o3;

    o1.writable = 10;
    o2.writable = 20;
    o3.vpReadable.updateData(Variant(30));

    auto bo12 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_TRUE(bo12->isEnabled());
    EXPECT_EQ(20, o1.writable);
    EXPECT_EQ(30, o3.readable);

    auto bo13 = PropertyBinding::bindPermanent(o1.writable, o3.readable);
    EXPECT_TRUE(bo13->isEnabled());
    EXPECT_FALSE(bo12->isEnabled());
    EXPECT_EQ(30, o1.writable);
    EXPECT_EQ(20, o2.writable);

    // Write to o2, it does not update o1.
    o2.writable = 200;
    EXPECT_EQ(30, o1.writable);

    // Enable bo12.
    bo12->setEnabled(true);
    EXPECT_EQ(200, o1.writable);

    // Update o2.
    o2.writable = 101;
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);

    // This shall make bo13 enabled.
    bo12->detach();
    EXPECT_EQ(30, o1.writable);

    o2.writable = 202;
    EXPECT_EQ(30, o1.writable);
    EXPECT_EQ(202, o2.writable);
    EXPECT_TRUE(bo13->isEnabled());
}

TEST_F(Bindings, test_binding_in_row)
{
    WritableTest o1, o2, o3;

    auto b1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto b2 = PropertyBinding::bindPermanent(o2.writable, o3.writable);
    auto b3 = PropertyBinding::bindPermanent(o3.writable, o1.writable);

    o1.writable = 1;
    EXPECT_EQ(1, o1.writable);
    EXPECT_EQ(1, o2.writable);
    EXPECT_EQ(1, o3.writable);

    o2.writable = 2;
    EXPECT_EQ(2, o1.writable);
    EXPECT_EQ(2, o2.writable);
    EXPECT_EQ(2, o3.writable);

    o3.writable = 3;
    EXPECT_EQ(3, o1.writable);
    EXPECT_EQ(3, o2.writable);
    EXPECT_EQ(3, o3.writable);
}

TEST_F(Bindings, test_group_bindings_explicitly)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto group = BindingGroup::create();
    EXPECT_NOT_NULL(group);
    EXPECT_TRUE(group->isEmpty());
    EXPECT_EQ(0u, group->getBindingCount());

    group->addBinding(*PropertyBinding::bindPermanent(o1.writable, o3.readable));
    group->addBinding(*PropertyBinding::bindAutoDiscard(o1.writable, o2.writable));

    EXPECT_EQ(2u, group->getBindingCount());
    auto b1 = (*group)[0];
    EXPECT_NOT_NULL(b1);
    EXPECT_EQ(&o1.writable, b1->getTarget());
    EXPECT_TRUE(b1->isAttached());
    EXPECT_FALSE(b1->isEnabled());
    EXPECT_TRUE(b1->isPermanent());

    auto b2 = (*group)[1];
    EXPECT_NOT_NULL(b2);
    EXPECT_EQ(&o1.writable, b2->getTarget());
    EXPECT_TRUE(b2->isAttached());
    EXPECT_TRUE(b2->isEnabled());
    EXPECT_FALSE(b2->isPermanent());
}

TEST_F(Bindings, test_group_explicitly_created_with_mixed_property_binding_types_discards)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto group = BindingGroup::create();
    EXPECT_NOT_NULL(group);
    EXPECT_TRUE(group->isEmpty());
    EXPECT_EQ(0u, group->getBindingCount());

    group->addBinding(*PropertyBinding::bindPermanent(o1.writable, o3.readable));
    group->addBinding(*PropertyBinding::bindAutoDiscard(o1.writable, o2.writable));
    EXPECT_FALSE((*group)[0]->isEnabled());
    EXPECT_TRUE((*group)[1]->isEnabled());

    // Enable b1, and write to the target. Write operation removes all discardable bindings from
    // the target. Both bindings on the target being grouped, the group removes the permanent binding too.
    (*group)[0]->setEnabled(true);
    EXPECT_TRUE((*group)[0]->isEnabled());
    EXPECT_FALSE((*group)[1]->isEnabled());

    o1.writable = 1;
    EXPECT_FALSE((*group)[0]->isAttached());
    EXPECT_FALSE((*group)[1]->isAttached());
}

TEST_F(Bindings, test_empty_arguments)
{
    auto group = BindingGroup::bindPermanent();
    EXPECT_NULL(group);
    group = BindingGroup::bindAutoDiscard();
    EXPECT_NULL(group);
    group = BindingGroup::bindPermanentCircular();
    EXPECT_NULL(group);
    group = BindingGroup::bindAutoDiscardCircular();
    EXPECT_NULL(group);
}

TEST_F(Bindings, test_binding_group_with_one_readonly_property_permanent)
{
    ReadableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanent(o1.readable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
}

TEST_F(Bindings, test_binding_groups_with_one_readonly_property_auto_discard)
{
    ReadableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindAutoDiscard(o1.readable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
}

TEST_F(Bindings, test_binding_group_with_one_readonly_property_permanent_circular)
{
    ReadableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanentCircular(o1.readable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
}

TEST_F(Bindings, test_binding_groups_with_one_readonly_property_auto_discard_circular)
{
    ReadableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindAutoDiscardCircular(o1.readable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
}

TEST_F(Bindings, test_binding_groups_with_two_readonly_property_fails)
{
    ReadableTest o1;
    ReadableTest o2;
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanent(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bindAutoDiscard(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bindPermanentCircular(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bindAutoDiscardCircular(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_permanent)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanent(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(2u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, (*group)[0]->getTarget());
    EXPECT_EQ(&o1.writable, (*group)[1]->getTarget());

    // Write to o3 updates all, but writes to o2 updates o1 only, and o1 write does not update any.
    o3.writable = 100;
    EXPECT_EQ(100, o2.writable);
    EXPECT_EQ(100, o1.writable);

    o2.writable = 200;
    EXPECT_EQ(100, o3.writable);
    EXPECT_EQ(200, o1.writable);

    o1.writable = 300;
    EXPECT_EQ(100, o3.writable);
    EXPECT_EQ(200, o2.writable);
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_permanent_circular)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanentCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, (*group)[0]->getTarget());
    EXPECT_EQ(&o1.writable, (*group)[1]->getTarget());
    EXPECT_EQ(&o3.writable, (*group)[2]->getTarget());

    // Writes to any property updates all.
    o1.writable = 100;
    EXPECT_EQ(100, o2.writable);
    EXPECT_EQ(100, o3.writable);
    o2.writable = 200;
    EXPECT_EQ(200, o1.writable);
    EXPECT_EQ(200, o3.writable);
    o3.writable = 300;
    EXPECT_EQ(300, o1.writable);
    EXPECT_EQ(300, o2.writable);
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_auto_discard)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindAutoDiscard(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(2u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, (*group)[0]->getTarget());
    EXPECT_EQ(&o1.writable, (*group)[1]->getTarget());

    // Write on any property detaches all the bindings
    o2.writable = 100;
    EXPECT_EQ(2, o1.writable);
    EXPECT_EQ(100, o2.writable);
    EXPECT_EQ(2, o3.writable);

    EXPECT_EQ(2u, group->getBindingCount());
    EXPECT_FALSE((*group)[0]->isAttached());
    EXPECT_FALSE((*group)[1]->isAttached());
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_auto_discard_circular)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindAutoDiscardCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, (*group)[0]->getTarget());
    EXPECT_EQ(&o1.writable, (*group)[1]->getTarget());
    EXPECT_EQ(&o3.writable, (*group)[2]->getTarget());

    // Write on any property detaches all the bindings
    o2.writable = 100;
    EXPECT_EQ(2, o1.writable);
    EXPECT_EQ(100, o2.writable);
    EXPECT_EQ(2, o3.writable);

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_FALSE((*group)[0]->isAttached());
    EXPECT_FALSE((*group)[1]->isAttached());
    EXPECT_FALSE((*group)[2]->isAttached());
}

TEST_F(Bindings, test_property_increment_keeps_permanent_bindings)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3(101);

    auto group = BindingGroup::bindPermanentCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);

    ++o2.writable;
    EXPECT_TRUE((*group)[0]->isAttached());
    EXPECT_TRUE((*group)[1]->isAttached());
    EXPECT_TRUE((*group)[2]->isAttached());
    EXPECT_EQ(102, o1.writable);
    EXPECT_EQ(102, o2.writable);
    EXPECT_EQ(102, o3.writable);

    --o1.writable;
    EXPECT_TRUE((*group)[0]->isAttached());
    EXPECT_TRUE((*group)[1]->isAttached());
    EXPECT_TRUE((*group)[2]->isAttached());
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);
}

TEST_F(Bindings, test_assign_property_present_in_binding_breaks_binding)
{
    WritableTest o1;
    WritableTest o2;
    WritableTest o3(101);

    auto group = BindingGroup::bindAutoDiscardCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_EQ(101, o1.writable);
    EXPECT_EQ(101, o2.writable);
    EXPECT_EQ(101, o3.writable);

    o1.writable = o3.writable;
    EXPECT_FALSE((*group)[0]->isAttached());
    EXPECT_FALSE((*group)[1]->isAttached());
    EXPECT_FALSE((*group)[2]->isAttached());
}


TEST_F(Bindings, test_expression_binding_create_permanent)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::create([&o2]() { return Variant(o2.writable + 2); }, true);
    EXPECT_NOT_NULL(binding);
    EXPECT_FALSE(binding->isEnabled());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_TRUE(binding->isPermanent());

    EXPECT_EQ(0, o1.writable);
    o1.writable.addBinding(binding);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(22, o1.writable);

    o2.writable = 30;
    EXPECT_EQ(32, o1.writable);

    o1.writable = 4;
    EXPECT_EQ(4, o1.writable);

    o2.writable = 3;
    EXPECT_EQ(5, o1.writable);
}

TEST_F(Bindings, test_expression_binding_create_discardable)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::create([&o2]() { return Variant(o2.writable + 2); }, false);
    EXPECT_NOT_NULL(binding);
    EXPECT_FALSE(binding->isEnabled());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_FALSE(binding->isPermanent());

    EXPECT_EQ(0, o1.writable);
    o1.writable.addBinding(binding);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(22, o1.writable);

    o2.writable = 30;
    EXPECT_EQ(32, o1.writable);

    o1.writable = 4;
    EXPECT_EQ(4, o1.writable);
    EXPECT_FALSE(binding->isAttached());

    o2.writable = 3;
    EXPECT_EQ(4, o1.writable);
}

TEST_F(Bindings, test_expression_binding_with_one_property_expression)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::bindPermanent(o1.writable, [&o2]() { return Variant(o2.writable * 2); });
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(40, o1.writable);

    o1.writable = 10;
    EXPECT_EQ(10, o1.writable);
    EXPECT_TRUE(binding->isAttached());

    o2.writable = 40;
    EXPECT_EQ(80, o1.writable);
}

TEST_F(Bindings, test_expression_binding_to_readonly_target_fails)
{
    ReadableTest o1;

    EXPECT_THROW(ExpressionBinding::bindPermanent(o1.readable, []() { return Variant(2); }), Exception);
}

TEST_F(Bindings, test_expression_binding_with_multiple_properties)
{
    WritableTest o1;
    WritableTest o2(2);
    WritableTest o3(3);

    auto binding = ExpressionBinding::bindPermanent(o1.writable, [&o2, &o3]() { return Variant(o2.writable * o3.writable); });
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(6, o1.writable);

    o2.writable = 10;
    EXPECT_EQ(30, o1.writable);

    o3.writable = o2.writable;
    EXPECT_EQ(100, o1.writable);
}

TEST_F(Bindings, test_expression_binding_conditional_with_multiple_properties)
{
    WritableTest o1;
    WritableTest o2(2);
    WritableTest o3(3);
    ReadableTest o4;

    auto expression = [&o2, &o3, &o4]()
    {
        if (o4.readable % 2)
        {
            return o3.writable.get();
        }
        return o2.writable.get();
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    // o4 is 99, this makes o1 to get o3 value.
    EXPECT_EQ(3, o1.writable);

    // update o4 to divide by 2. this makes o1 to get o2.
    o4.vpReadable.updateData(Variant(2));
    EXPECT_EQ(2, o1.writable);
}

TEST_F(Bindings, test_binding_detached_and_invalid_when_source_property_dies)
{
    WritableTest o1(1);
    auto binding = BindingSharedPtr();

    {
        WritableTest o2(2);
        binding = PropertyBinding::bindPermanent(o1.writable, o2.writable);
        EXPECT_TRUE(binding->isAttached());
    }

    EXPECT_FALSE(binding->isAttached());
    // try to re-attach the binding to o1
    EXPECT_THROW(o1.writable.addBinding(binding), Exception);
}

TEST_F(Bindings, test_expression_binding_detached_and_invalid_when_source_in_expression_dies)
{
    WritableTest o1(1);
    WritableTest o2(5);
    auto binding = BindingSharedPtr();
    {
        WritableTest o3(100);
        binding = ExpressionBinding::bindPermanent(o1.writable, [&o2, &o3]() { return Variant(o2.writable + o3.writable); });
        EXPECT_TRUE(binding->isAttached());
        EXPECT_EQ(105, o1.writable);
    }

    EXPECT_FALSE(binding->isAttached());
    // write to o2
    o2.writable = 10;

    // try to re-attach the binding to o1.
    EXPECT_THROW(o1.writable.addBinding(binding), Exception);
}

TEST_F(Bindings, test_espression_binding_detect_binding_loop)
{
    WritableTest o1(1);
    WritableTest o2(2);
    WritableTest o3(3);

    // o1 is bount to {o2 + 2}
    ExpressionBinding::bindPermanent(o1.writable, [&o2]() { return Variant(o2.writable + 2); });
    EXPECT_EQ(4, o1.writable);

    // o3 is bount to o1
    PropertyBinding::bindPermanent(o3.writable, o1.writable);
    EXPECT_EQ(4, o3.writable);

    // o2 is bount to o3. This closes the loop, and produces binding loop.
    EXPECT_THROW(PropertyBinding::bindPermanent(o2.writable, o3.writable), Exception);
}

TEST_F(Bindings, test_remove_binding_from_wrong_target)
{
    WritableTest o1;
    WritableTest o2;

    auto b1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto b2 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    EXPECT_NOT_NULL(b1);
    EXPECT_NOT_NULL(b2);

    EXPECT_THROW(o1.writable.removeBinding(*b2), Exception);
    EXPECT_THROW(o2.writable.removeBinding(*b1), Exception);
}

TEST_F(Bindings, test_property_binding_becomes_invalid_before_being_attached)
{
    WritableTest o1;
    auto binding = BindingSharedPtr();

    {
        WritableTest o2(10);
        binding = PropertyBinding::create(o2.writable, true);
        EXPECT_TRUE(binding->isValid());
        EXPECT_FALSE(binding->isAttached());
    }

    EXPECT_FALSE(binding->isValid());
}

TEST_F(Bindings, test_expression_binding_becomes_invalid_before_being_attached)
{
    WritableTest o1;
    auto binding = BindingSharedPtr();

    {
        WritableTest o2(10);
        binding = ExpressionBinding::create([&o2]() { return o2.writable.get(); }, true);
        EXPECT_TRUE(binding->isValid());
        EXPECT_FALSE(binding->isAttached());
    }

    EXPECT_FALSE(binding->isValid());
}
