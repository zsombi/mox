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
#include <mox/core/meta/property/binding/binding.hpp>
#include <mox/core/meta/property/binding/binding_normalizer.hpp>
#include <mox/core/meta/property/binding/binding_group.hpp>
#include <mox/core/meta/property/binding/property_binding.hpp>
#include <mox/core/meta/property/binding/expression_binding.hpp>
#include <mox/core/object.hpp>

using namespace mox;

template <class LockableObject>
class WritablePropertyHolder
{
    PropertyData<int> writableValue{0};
public:
    static inline SignalTypeDecl<int> WritablePropertyChangedSignalType;
    static inline PropertyTypeDecl<int, PropertyAccess::ReadWrite> WritablePropertyType = {WritablePropertyChangedSignalType};
    Property writable;

    explicit WritablePropertyHolder(LockableObject& lockable)
        : writable(lockable, WritablePropertyType, writableValue)
    {
    }
};

class WritableTest : public Object, public WritablePropertyHolder<WritableTest>
{
public:

    explicit WritableTest(int initialValue = 0)
        : WritablePropertyHolder<WritableTest>(static_cast<WritableTest&>(*this))
    {
        writable = initialValue;
    }
};

class ReadableTest : public Object
{
public:
    UpdatingPropertyData<int> vpReadable = 99;

    static inline SignalTypeDecl<int> ReadablePropertyChangedSignalType;
    static inline PropertyTypeDecl<int, PropertyAccess::ReadOnly> ReadablePropertyType = {ReadablePropertyChangedSignalType, 99};
    Property readable{*this, ReadablePropertyType, vpReadable};

    explicit ReadableTest() = default;
};

class BindingThread : public TestThreadLoop, public WritablePropertyHolder<BindingThread>
{
    static inline ThreadPromise dummyDeath;
    static inline mox::ThreadFuture dummyWatch;
protected:
    explicit BindingThread(ThreadPromise&& notifier)
        : TestThreadLoop(std::forward<ThreadPromise>(notifier))
        , WritablePropertyHolder<BindingThread>(static_cast<BindingThread&>(*this))
    {
        writable = 20;
    }

    void initialize() override
    {
        TestThreadLoop::initialize();
        addEventHandler(evUpdate, std::bind(&BindingThread::onUpdateEvent, this, std::placeholders::_1));
    }

    void onUpdateEvent(Event&)
    {
        CTRACE(event, "trigger binding");
        int v = writable;
        writable = ++v;
    }

public:
    static inline EventType const evUpdate = Event::registerNewType();

    static std::shared_ptr<BindingThread> create()
    {
        dummyDeath = ThreadPromise();
        dummyWatch = dummyDeath.get_future();

        return make_thread(new BindingThread(std::move(dummyDeath)));
    }
};



class Bindings : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();

        registerMetaClass<BindingThread>();
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

    auto binding2 = PropertyBinding::bind(o2.writable, o1.writable);
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

    auto binding = PropertyBinding::bind(o1.writable, o2.writable);
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
    EXPECT_EQ(99, int(o1.writable));
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
    EXPECT_EQ(99, int(o1.writable));

    binding1->setEnabled(true);
    EXPECT_TRUE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(20, int(o1.writable));

    binding2->setEnabled(true);
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_TRUE(binding2->isEnabled());
    EXPECT_EQ(99, int(o1.writable));
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
    EXPECT_EQ(99, int(o1.writable));

    binding1->setEnabled(true);
    EXPECT_TRUE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(99, int(o1.writable));
}

TEST_F(Bindings, test_disable_all_bindings)
{
    WritableTest o1(10);
    WritableTest o2(20);
    ReadableTest o3;

    auto binding1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o3.readable);

    EXPECT_EQ(99, int(o1.writable));

    binding2->setEnabled(false);
    EXPECT_FALSE(binding1->isEnabled());
    EXPECT_FALSE(binding2->isEnabled());
    EXPECT_EQ(99, int(o1.writable));

    // update readable
    o3.vpReadable.update(1000);
    EXPECT_EQ(99, int(o1.writable));

    o2.writable = 1;
    EXPECT_EQ(99, int(o1.writable));
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
    EXPECT_EQ(99, int(o1.writable));
    EXPECT_EQ(1, o1ChangeCount);
    EXPECT_EQ(0, o2ChangeCount);

    // Update o2 property. This will change o1
    o2.vpReadable.update(101);
    EXPECT_EQ(101, int(o1.writable));
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
    EXPECT_EQ(99, int(o1.writable));

    binding->detach();
    EXPECT_FALSE(binding->isAttached());

    // Update o2 property. This will no longer change o1, as the binding is removed.
    o2.vpReadable.update(101);
    EXPECT_EQ(99, int(o1.writable));
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
    EXPECT_EQ(10, int(o2.writable));

    // o2 write does not update o1, only the other way around.
    o1.writable = 5;
    EXPECT_EQ(5, int(o2.writable));
    o2.writable = 9;
    EXPECT_EQ(5, int(o1.writable));
    EXPECT_EQ(9, int(o2.writable));

    // Create binding the other way around
    auto binding2 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_NOT_NULL(binding2);
    EXPECT_TRUE(binding2->isAttached());
    EXPECT_EQ(&o1.writable, binding2->getTarget());
    EXPECT_EQ(9, int(o1.writable));

    // Writes to either of the property will update both
    o1.writable = 0;
    EXPECT_EQ(0, int(o2.writable));
    o2.writable = 99;
    EXPECT_EQ(99, int(o1.writable));
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
    EXPECT_EQ(10, int(o2.writable));
    o2.writable = 80;
    EXPECT_EQ(80, int(o1.writable));

    binding2->detach();
    o1.writable = 100;
    EXPECT_EQ(80, int(o2.writable));
    o2.writable = 80;
    EXPECT_EQ(100, int(o1.writable));
}

TEST_F(Bindings, test_multiple_permanent_bindings_on_target)
{
    WritableTest o1;
    WritableTest o2;
    ReadableTest o3;

    o1.writable = 10;
    o2.writable = 20;
    o3.vpReadable.update(30);

    auto bo12 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    EXPECT_TRUE(bo12->isEnabled());
    EXPECT_EQ(20, int(o1.writable));
    EXPECT_EQ(30, int(o3.readable));

    auto bo13 = PropertyBinding::bindPermanent(o1.writable, o3.readable);
    EXPECT_TRUE(bo13->isEnabled());
    EXPECT_FALSE(bo12->isEnabled());
    EXPECT_EQ(30, int(o1.writable));
    EXPECT_EQ(20, int(o2.writable));

    // Write to o2, it does not update o1.
    o2.writable = 200;
    EXPECT_EQ(30, int(o1.writable));

    // Enable bo12.
    bo12->setEnabled(true);
    EXPECT_EQ(200, int(o1.writable));

    // Update o2.
    o2.writable = 101;
    EXPECT_EQ(101, int(o1.writable));
    EXPECT_EQ(101, int(o2.writable));

    // This shall make bo13 enabled.
    bo12->detach();
    EXPECT_EQ(30, int(o1.writable));

    o2.writable = 202;
    EXPECT_EQ(30, int(o1.writable));
    EXPECT_EQ(202, int(o2.writable));
    EXPECT_TRUE(bo13->isEnabled());
}

TEST_F(Bindings, test_binding_in_row)
{
    WritableTest o1, o2, o3;

    auto b1 = PropertyBinding::bindPermanent(o1.writable, o2.writable);
    auto b2 = PropertyBinding::bindPermanent(o2.writable, o3.writable);
    auto b3 = PropertyBinding::bindPermanent(o3.writable, o1.writable);

    o1.writable = 1;
    EXPECT_EQ(1, int(o1.writable));
    EXPECT_EQ(1, int(o2.writable));
    EXPECT_EQ(1, int(o3.writable));

    o2.writable = 2;
    EXPECT_EQ(2, int(o1.writable));
    EXPECT_EQ(2, int(o2.writable));
    EXPECT_EQ(2, int(o3.writable));

    o3.writable = 3;
    EXPECT_EQ(3, int(o1.writable));
    EXPECT_EQ(3, int(o2.writable));
    EXPECT_EQ(3, int(o3.writable));
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
    group->addBinding(*PropertyBinding::bind(o1.writable, o2.writable));

    EXPECT_EQ(2u, group->getBindingCount());
    auto b1 = group->at(0);
    EXPECT_NOT_NULL(b1);
    EXPECT_EQ(&o1.writable, b1->getTarget());
    EXPECT_TRUE(b1->isAttached());
    EXPECT_FALSE(b1->isEnabled());
    EXPECT_TRUE(b1->isPermanent());

    auto b2 = group->at(1);
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
    group->addBinding(*PropertyBinding::bind(o1.writable, o2.writable));
    EXPECT_FALSE(group->at(0)->isEnabled());
    EXPECT_TRUE(group->at(1)->isEnabled());

    // Enable b1, and write to the target. Write operation removes all discardable bindings from
    // the target. Both bindings on the target being grouped, the group removes the permanent binding too.
    group->at(0)->setEnabled(true);
    EXPECT_TRUE(group->at(0)->isEnabled());
    EXPECT_FALSE(group->at(1)->isEnabled());

    o1.writable = 1;
    EXPECT_FALSE(group->at(0)->isAttached());
    EXPECT_FALSE(group->at(1)->isAttached());
}

TEST_F(Bindings, test_empty_arguments)
{
    auto group = BindingGroup::bindPermanent();
    EXPECT_NULL(group);
    group = BindingGroup::bind();
    EXPECT_NULL(group);
    group = BindingGroup::bindPermanentCircular();
    EXPECT_NULL(group);
    group = BindingGroup::bindCircular();
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

    auto group = BindingGroup::bind(o1.readable, o2.writable, o3.writable);
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

    auto group = BindingGroup::bindCircular(o1.readable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);
}

TEST_F(Bindings, test_binding_groups_with_two_readonly_property_fails)
{
    ReadableTest o1;
    ReadableTest o2;
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanent(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bind(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bindPermanentCircular(o1.readable, o2.readable, o3.writable);
    EXPECT_NULL(group);
    group = BindingGroup::bindCircular(o1.readable, o2.readable, o3.writable);
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
    EXPECT_EQ(&o2.writable, group->at(0)->getTarget());
    EXPECT_EQ(&o1.writable, group->at(1)->getTarget());

    // Write to o3 updates all, but writes to o2 updates o1 only, and o1 write does not update any.
    o3.writable = 100;
    EXPECT_EQ(100, int(o2.writable));
    EXPECT_EQ(100, int(o1.writable));

    o2.writable = 200;
    EXPECT_EQ(100, int(o3.writable));
    EXPECT_EQ(200, int(o1.writable));

    o1.writable = 300;
    EXPECT_EQ(100, int(o3.writable));
    EXPECT_EQ(200, int(o2.writable));
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_permanent_circular)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindPermanentCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, group->at(0)->getTarget());
    EXPECT_EQ(&o1.writable, group->at(1)->getTarget());
    EXPECT_EQ(&o3.writable, group->at(2)->getTarget());

    // Writes to any property updates all.
    o1.writable = 100;
    EXPECT_EQ(100, int(o2.writable));
    EXPECT_EQ(100, int(o3.writable));
    o2.writable = 200;
    EXPECT_EQ(200, int(o1.writable));
    EXPECT_EQ(200, int(o3.writable));
    o3.writable = 300;
    EXPECT_EQ(300, int(o1.writable));
    EXPECT_EQ(300, int(o2.writable));
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_auto_discard)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bind(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(2u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, group->at(0)->getTarget());
    EXPECT_EQ(&o1.writable, group->at(1)->getTarget());

    // Write on any property detaches all the bindings
    o2.writable = 100;
    EXPECT_EQ(2, int(o1.writable));
    EXPECT_EQ(100, int(o2.writable));
    EXPECT_EQ(2, int(o3.writable));

    EXPECT_EQ(2u, group->getBindingCount());
    EXPECT_FALSE(group->at(0)->isAttached());
    EXPECT_FALSE(group->at(1)->isAttached());
}

TEST_F(Bindings, test_binding_groups_with_writable_properties_auto_discard_circular)
{
    WritableTest o1;
    WritableTest o2(1);
    WritableTest o3(2);

    auto group = BindingGroup::bindCircular(o1.writable, o2.writable, o3.writable);
    EXPECT_NOT_NULL(group);

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_EQ(&o2.writable, group->at(0)->getTarget());
    EXPECT_EQ(&o1.writable, group->at(1)->getTarget());
    EXPECT_EQ(&o3.writable, group->at(2)->getTarget());

    // Write on any property detaches all the bindings
    o2.writable = 100;
    EXPECT_EQ(2, int(o1.writable));
    EXPECT_EQ(100, int(o2.writable));
    EXPECT_EQ(2, int(o3.writable));

    EXPECT_EQ(3u, group->getBindingCount());
    EXPECT_FALSE(group->at(0)->isAttached());
    EXPECT_FALSE(group->at(1)->isAttached());
    EXPECT_FALSE(group->at(2)->isAttached());
}


TEST_F(Bindings, test_expression_binding_create_permanent)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::create([&o2]() { return Variant(int(o2.writable) + 2); }, true);
    EXPECT_NOT_NULL(binding);
    EXPECT_FALSE(binding->isEnabled());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_TRUE(binding->isPermanent());

    EXPECT_EQ(0, int(o1.writable));
    binding->attach(o1.writable);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(22, int(o1.writable));

    o2.writable = 30;
    EXPECT_EQ(32, int(o1.writable));

    o1.writable = 4;
    EXPECT_EQ(4, int(o1.writable));

    o2.writable = 3;
    EXPECT_EQ(5, int(o1.writable));
}

TEST_F(Bindings, test_expression_binding_create_discardable)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::create([&o2]() { return Variant(int(o2.writable) + 2); }, false);
    EXPECT_NOT_NULL(binding);
    EXPECT_FALSE(binding->isEnabled());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_FALSE(binding->isPermanent());

    EXPECT_EQ(0, int(o1.writable));
    binding->attach(o1.writable);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(22, int(o1.writable));

    o2.writable = 30;
    EXPECT_EQ(32, int(o1.writable));

    o1.writable = 4;
    EXPECT_EQ(4, int(o1.writable));
    EXPECT_FALSE(binding->isAttached());

    o2.writable = 3;
    EXPECT_EQ(4, int(o1.writable));
}

TEST_F(Bindings, test_expression_binding_with_one_property_expression)
{
    WritableTest o1;
    WritableTest o2(20);

    auto binding = ExpressionBinding::bindPermanent(o1.writable, [&o2]() { return Variant(int(o2.writable) * 2); });
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isEnabled());
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(40, int(o1.writable));

    o1.writable = 10;
    EXPECT_EQ(10, int(o1.writable));
    EXPECT_TRUE(binding->isAttached());

    o2.writable = 40;
    EXPECT_EQ(80, int(o1.writable));
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

    auto binding = ExpressionBinding::bindPermanent(o1.writable, [&o2, &o3]() { return Variant(int(o2.writable) * int(o3.writable)); });
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    EXPECT_EQ(6, int(o1.writable));

    o2.writable = 10;
    EXPECT_EQ(30, int(o1.writable));

    o3.writable = int(o2.writable);
    EXPECT_EQ(100, int(o1.writable));
}

TEST_F(Bindings, test_expression_binding_conditional_with_multiple_properties)
{
    WritableTest o1;
    WritableTest o2(2);
    WritableTest o3(3);
    ReadableTest o4;

    auto expression = [&o2, &o3, &o4]()
    {
        if (int(o4.readable) % 2)
        {
            return o3.writable.get();
        }
        return o2.writable.get();
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);
    EXPECT_NOT_NULL(binding);
    EXPECT_TRUE(binding->isAttached());
    // o4 is 99, this makes o1 to get o3 value.
    EXPECT_EQ(3, int(o1.writable));

    // update o4 to divide by 2. this makes o1 to get o2.
    o4.vpReadable.update(2);
    EXPECT_EQ(2, int(o1.writable));
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
    EXPECT_THROW(binding->attach(o1.writable), Exception);
}

TEST_F(Bindings, test_expression_binding_detached_and_invalid_when_source_in_expression_dies)
{
    WritableTest o1(1);
    WritableTest o2(5);
    auto binding = BindingSharedPtr();
    {
        WritableTest o3(100);
        binding = ExpressionBinding::bindPermanent(o1.writable, [&o2, &o3]() { return Variant(int(o2.writable) + int(o3.writable)); });
        EXPECT_TRUE(binding->isAttached());
        EXPECT_EQ(105, int(o1.writable));
    }

    EXPECT_FALSE(binding->isAttached());
    // write to o2
    o2.writable = 10;

    // try to re-attach the binding to o1.
    EXPECT_THROW(binding->attach(o1.writable), Exception);
}

TEST_F(Bindings, test_espression_binding_detect_binding_loop)
{
    WritableTest o1(1);
    WritableTest o2(2);
    WritableTest o3(3);

    // o1 is bount to {o2 + 2}
    ExpressionBinding::bindPermanent(o1.writable, [&o2]() { return Variant(int(o2.writable) + 2); });
    EXPECT_EQ(4, int(o1.writable));

    // o3 is bount to o1
    PropertyBinding::bindPermanent(o3.writable, o1.writable);
    EXPECT_EQ(4, int(o3.writable));

    // o2 is bount to o3. This closes the loop, and produces binding loop.
    EXPECT_THROW(PropertyBinding::bindPermanent(o2.writable, o3.writable), Exception);
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


TEST_F(Bindings, test_binding_loop_detection_needs_higher_count)
{
    class SimpleNormalizer : public BindingNormalizer
    {
    public:
        size_t& callCount;
    public:
        explicit SimpleNormalizer(size_t& callCount)
            : callCount(callCount)
        {
        }
        Result tryNormalize(Binding& binding, Variant&, size_t loopCount) override
        {
            if (binding.shared_from_this() != getTarget())
            {
                return Normalized;
            }
            ++callCount;
            return (loopCount <= 2) ? Normalized : Throw;
        }
    };

    WritableTest o1;
    WritableTest o2(10);

    auto b1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    auto expression = [&o2]()
    {
        return Variant(int(o2.writable) % 3);
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);

    // This may be a binding loop, however a loop count of 2 would normalize it.
    EXPECT_THROW(o1.writable = 3, Exception);
    o1.writable = 1;

    size_t normalizerCallCount= 0u;
    auto group = BindingGroup::create(binding, b1);
    group->setNormalizer(*binding, std::make_unique<SimpleNormalizer>(normalizerCallCount));

    // store intermediate values of o1
    std::vector<int> o1Values;
    o1.writable.changed.connect([&o1Values](int value) { o1Values.push_back(value); });

    // Try the binding now.
    EXPECT_NO_THROW(o1.writable = 3);
    EXPECT_EQ(1, normalizerCallCount);
    EXPECT_EQ(2u, o1Values.size());
    EXPECT_EQ(0, o1Values[0]);
    EXPECT_EQ(3, o1Values[1]);
}

TEST_F(Bindings, test_binding_loop_normalizer_changes_value)
{
    WritableTest o1;
    WritableTest o2(10);

    class ComplexNormalizer : public BindingNormalizer
    {
        size_t& callCount;
    public:
        explicit ComplexNormalizer(size_t& callCount)
            : callCount(callCount)
        {
        }
        Result tryNormalize(Binding& binding, Variant& value, size_t loopCount) override
        {
            if (binding.shared_from_this() != getTarget())
            {
                return Normalized;
            }
            ++callCount;
            if (loopCount == 2)
            {
                value = 1;
            }
            if (loopCount == 3)
            {
                value = 2;
            }
            return (loopCount <= 5) ? Normalized : Throw;
        }
    };

    auto b1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    auto expression = [&o2]()
    {
        return Variant(int(o2.writable) % 3);
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);

    // This may be a binding loop, however a loop count of 2 would normalize it.
    EXPECT_THROW(o1.writable = 3, Exception);
    o1.writable = 1;

    size_t normalizerCallCount= 0u;
    auto group = BindingGroup::create(binding, b1);
    group->setNormalizer(*binding, std::make_unique<ComplexNormalizer>(normalizerCallCount));

    // store intermediate values of o1
    std::vector<int> o1Values;
    o1.writable.changed.connect([&o1Values](int value) { o1Values.push_back(value); });

    // Try the binding now.
    EXPECT_NO_THROW(o1.writable = 3);
    EXPECT_EQ(3, normalizerCallCount);
    EXPECT_EQ(4u, o1Values.size());
    EXPECT_EQ(2, o1Values[0]);
    EXPECT_EQ(1, o1Values[1]);
    EXPECT_EQ(0, o1Values[2]);
    EXPECT_EQ(3, o1Values[3]);
}

TEST_F(Bindings, test_binding_loop_normalize_throw_if_normalization_fails_after_4_loop_count)
{
    WritableTest o1;
    WritableTest o2(10);

    class ComplexNormalizer : public BindingNormalizer
    {
        size_t& callCount;
    public:
        explicit ComplexNormalizer(size_t& callCount)
            : callCount(callCount)
        {
        }
        Result tryNormalize(Binding& binding, Variant& value, size_t loopCount) override
        {
            if (binding.shared_from_this() != getTarget())
            {
                return Normalized;
            }
            ++callCount;
            switch (loopCount)
            {
                case 2:
                {
                    value = 1;
                    return Normalized;
                }
                case 3:
                {
                    value = 2;
                    return Normalized;
                }
                default:
                {
                    return Throw;
                }
            }
        }
    };

    auto b1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    auto expression = [&o2]()
    {
        return Variant(int(o2.writable) % 3);
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);

    size_t normalizerCallCount= 0u;
    auto group = BindingGroup::create(binding, b1);
    group->setNormalizer(*binding, std::make_unique<ComplexNormalizer>(normalizerCallCount));

    // store intermediate values of o1
    std::vector<int> o1Values;
    o1.writable.changed.connect([&o1Values](int value) { o1Values.push_back(value); });

    // The expression should throw
    EXPECT_THROW(o1.writable = 3, Exception);
    EXPECT_EQ(3, normalizerCallCount);
    EXPECT_EQ(0u, o1Values.size());
}

TEST_F(Bindings, test_binding_loop_normalize_exit_if_normalization_fails_after_4_loop_count)
{
    WritableTest o1;
    WritableTest o2(10);

    class ComplexNormalizer : public BindingNormalizer
    {
        size_t& callCount;
    public:
        explicit ComplexNormalizer(size_t& callCount)
            : callCount(callCount)
        {
        }
        Result tryNormalize(Binding& binding, Variant& value, size_t loopCount) override
        {
            if (binding.shared_from_this() != getTarget())
            {
                return Normalized;
            }
            ++callCount;
            switch (loopCount)
            {
                case 2:
                {
                    value = 1;
                    return Normalized;
                }
                case 3:
                {
                    value = 2;
                    return Normalized;
                }
                default:
                {
                    return FailAndExit;
                }
            }
        }
    };

    auto b1 = PropertyBinding::bindPermanent(o2.writable, o1.writable);
    auto expression = [&o2]()
    {
        return Variant(int(o2.writable) % 3);
    };
    auto binding = ExpressionBinding::bindPermanent(o1.writable, expression);

    size_t normalizerCallCount= 0u;
    auto group = BindingGroup::create(binding, b1);
    group->setNormalizer(*binding, std::make_unique<ComplexNormalizer>(normalizerCallCount));

    // store intermediate values of o1
    std::vector<int> o1Values;
    o1.writable.changed.connect([&o1Values](int value) { o1Values.push_back(value); });

    // The expression should throw
    EXPECT_NO_THROW(o1.writable = 3);
    EXPECT_EQ(3, normalizerCallCount);
    EXPECT_EQ(4u, o1Values.size());
    EXPECT_EQ(2, o1Values[0]);
    EXPECT_EQ(1, o1Values[1]);
    EXPECT_EQ(0, o1Values[2]);
    EXPECT_EQ(3, o1Values[3]);
}

TEST_F(Bindings, test_property_binding_between_threads)
{
    WritableTest o1(10);
    auto binding = BindingSharedPtr();

    TestApp app;

    std::function<void()> kickThreadUpdate;
    std::function<void()> killThread;

    auto weakThread = std::weak_ptr<ThreadLoop>();
    {
        auto thread = BindingThread::create();
        weakThread = thread;
        EXPECT_EQ(20, int(thread->writable));

        binding = PropertyBinding::bindPermanent(o1.writable, thread->writable);
        EXPECT_TRUE(binding->isAttached());
        EXPECT_EQ(20, int(o1.writable));

        // start the thread
        thread->start();

        kickThreadUpdate = [wthread = std::weak_ptr<BindingThread>(thread)]()
        {
            auto thread = wthread.lock();
            EXPECT_NOT_NULL(thread);
            EXPECT_TRUE(postEvent<Event>(thread, BindingThread::evUpdate));
        };
        killThread = [wthread = std::weak_ptr<BindingThread>(thread)]()
        {
            auto thread = wthread.lock();
            EXPECT_NOT_NULL(thread);
            thread->exitAndJoin(0);
        };
    }

    // Wait for an update from the o1 property change.
    ThreadPromise updated;
    mox::ThreadFuture updateWatch = updated.get_future();
    auto onWritableChanged = [&updated]()
    {
        updated.set_value();
    };
    o1.writable.changed.connect(onWritableChanged);
    kickThreadUpdate();
    updateWatch.wait();

    EXPECT_EQ(21, int(o1.writable));

    // Stop the thread.
    killThread();

    // As there are no shared pointers holding the thread, and because the thread is stopped with exitAndJoin(),
    // the thread is destroyed, detaching all bindings to its properties.
    EXPECT_TRUE(weakThread.expired());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_FALSE(binding->isValid());
    // Run once the app. This will stop wrap up the root object, to which all orphan threads are parented.
    app.runOnce();
    // The binding is no longer valid, nor attached.
    EXPECT_FALSE(binding->isValid());
    EXPECT_FALSE(binding->isAttached());
}

TEST_F(Bindings, test_expression_binding_between_threads)
{
    WritableTest o1(10);
    auto binding = BindingSharedPtr();

    TestApp app;

    std::function<void()> kickThreadUpdate;
    std::function<void()> killThread;

    auto weakThread = std::weak_ptr<ThreadLoop>();
    {
        auto thread = BindingThread::create();
        weakThread = thread;
        EXPECT_EQ(20, int(thread->writable));
        // Note:!! Beware of the capture of the expression lambda!! Do not copy the thread handler as the lambda will hold the pointer reference!
        binding = ExpressionBinding::bindPermanent(o1.writable, [thrd = thread.get()]() { return thrd->writable.get(); });
        EXPECT_TRUE(binding->isAttached());
        EXPECT_EQ(20, int(o1.writable));

        // start the thread
        thread->start();

        kickThreadUpdate = [wthread = std::weak_ptr<BindingThread>(thread)]()
        {
            auto thread = wthread.lock();
            EXPECT_NOT_NULL(thread);
            postEvent<Event>(thread, BindingThread::evUpdate);
        };
        killThread = [wthread = std::weak_ptr<BindingThread>(thread)]()
        {
            auto thread = wthread.lock();
            EXPECT_NOT_NULL(thread);
            thread->exitAndJoin(0);
        };
    }

    // Wait for an update from the o1 property change.
    ThreadPromise updated;
    mox::ThreadFuture updateWatch = updated.get_future();
    auto onWritableChanged = [&updated]()
    {
        updated.set_value();
    };
    o1.writable.changed.connect(onWritableChanged);
    kickThreadUpdate();
    updateWatch.wait();

    EXPECT_EQ(21, int(o1.writable));

    // Stop the thread.
    killThread();

    // As there are no shared pointers holding the thread, and because the thread is stopped with exitAndJoin(),
    // the thread is destroyed, detaching all bindings to its properties.
    EXPECT_TRUE(weakThread.expired());
    EXPECT_FALSE(binding->isAttached());
    EXPECT_FALSE(binding->isValid());
    // Run once the app. This will stop wrap up the root object, to which all orphan threads are parented.
    app.runOnce();
    // The binding is no longer valid, nor attached.
    EXPECT_FALSE(binding->isValid());
    EXPECT_FALSE(binding->isAttached());
}

static SignalTypeDecl<int> RuntimeChangedSignalType;
static PropertyTypeDecl<int, PropertyAccess::ReadWrite> RuntimePropertyType = {RuntimeChangedSignalType};

TEST_F(Bindings, test_property_binding_with_runtime_property)
{
    WritableTest target;
    MetaBase source;

    auto runtime = source.setProperty(RuntimePropertyType, 12);
    auto binding = PropertyBinding::bindPermanent(target.writable, *runtime);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(12, int(target.writable));

    *runtime = 100;
    EXPECT_EQ(100, int(target.writable));
}

TEST_F(Bindings, test_expression_binding_with_runtime_property)
{
    WritableTest target;
    MetaBase source;

    auto runtime = source.setProperty(RuntimePropertyType, 12);
    auto binding = ExpressionBinding::bindPermanent(target.writable, [runtime]() { return Variant(int(*runtime) * 10); });
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(120, int(target.writable));

    *runtime = 100;
    EXPECT_EQ(1000, int(target.writable));
}

TEST_F(Bindings, test_property_binding_to_runtime_property)
{
    WritableTest source;
    MetaBase target;

    auto runtime = target.setProperty(RuntimePropertyType, 12);
    auto binding = PropertyBinding::bindPermanent(*runtime, source.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(0, int(*runtime));

    source.writable = 100;
    EXPECT_EQ(100, int(*runtime));
}

TEST_F(Bindings, test_expression_binding_to_runtime_property)
{
    WritableTest source;
    MetaBase target;

    auto runtime = target.setProperty(RuntimePropertyType, 12);
    auto binding = ExpressionBinding::bindPermanent(*runtime, [&source]() { return Variant(int(source.writable) * 10); });
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(0, int(*runtime));

    source.writable = 100;
    EXPECT_EQ(1000, int(*runtime));
}

TEST_F(Bindings, test_binding_detaches_when_target_is_deleted)
{
    auto target = std::make_shared<MetaBase>();
    WritableTest source(100);

    auto runtime = target->setProperty(RuntimePropertyType, 12);
    EXPECT_EQ(12, int(*runtime));

    auto binding = PropertyBinding::bindPermanent(*runtime, source.writable);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(100, int(*runtime));

    target.reset();
    EXPECT_EQ(BindingState::Detached, binding->getState());
}

TEST_F(Bindings, test_binding_invalid_when_source_is_deleted)
{
    auto source = std::make_shared<MetaBase>();
    WritableTest target;

    auto runtime = source->setProperty(RuntimePropertyType, 12);
    EXPECT_EQ(12, int(*runtime));

    auto binding = PropertyBinding::bindPermanent(target.writable, *runtime);
    EXPECT_NOT_NULL(binding);
    EXPECT_EQ(12, int(target.writable));

    source.reset();
    EXPECT_EQ(BindingState::Invalid, binding->getState());
}

TEST_F(Bindings, test_bind_to_invalid_runtime_property)
{
    auto target = std::make_shared<MetaBase>();
    auto p = target->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    target.reset();

    WritableTest source;
    EXPECT_THROW(PropertyBinding::bindPermanent(*runtime, source.writable), Exception);
}

TEST_F(Bindings, test_bind_with_invalid_runtime_property)
{
    auto source = std::make_shared<MetaBase>();
    auto p = source->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    source.reset();

    WritableTest target;
    EXPECT_THROW(PropertyBinding::bindPermanent(target.writable, *runtime), Exception);
}

TEST_F(Bindings, test_bind_autodiscard_to_invalid_runtime_property)
{
    auto target = std::make_shared<MetaBase>();
    auto p = target->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    target.reset();

    WritableTest source;
    EXPECT_THROW(PropertyBinding::bind(*runtime, source.writable), Exception);
}

TEST_F(Bindings, test_bind_autodiscard_with_invalid_runtime_property)
{
    auto source = std::make_shared<MetaBase>();
    auto p = source->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    source.reset();

    WritableTest target;
    EXPECT_THROW(PropertyBinding::bind(target.writable, *runtime), Exception);
}

TEST_F(Bindings, test_bind_expression_to_invalid_runtime_property)
{
    auto target = std::make_shared<MetaBase>();
    auto p = target->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    target.reset();

    WritableTest source;
    auto expression = [&source]() { return source.writable.get(); };
    EXPECT_THROW(ExpressionBinding::bindPermanent(*runtime, expression), Exception);
}

TEST_F(Bindings, test_bind_autodiscard_expression_to_invalid_runtime_property)
{
    auto target = std::make_shared<MetaBase>();
    auto p = target->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    target.reset();

    WritableTest source;
    auto expression = [&source]() { return source.writable.get(); };
    EXPECT_THROW(ExpressionBinding::bind(*runtime, expression), Exception);
}

TEST_F(Bindings, test_bind_expression_with_invalid_runtime_property)
{
    auto source = std::make_shared<MetaBase>();
    auto p = source->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    source.reset();

    WritableTest target;
    auto expression = [&runtime]() { return runtime->get(); };
    EXPECT_THROW(ExpressionBinding::bindPermanent(target.writable, expression), Exception);
}

TEST_F(Bindings, test_bind_autodetaching_expression_with_invalid_runtime_property)
{
    auto source = std::make_shared<MetaBase>();
    auto p = source->setProperty(RuntimePropertyType, 12);
    DynamicPropertyPtr runtime = dynamic_cast<DynamicProperty*>(p)->shared_from_this();
    source.reset();

    WritableTest target;
    auto expression = [&runtime]() { return runtime->get(); };
    EXPECT_THROW(ExpressionBinding::bind(target.writable, expression), Exception);
}

TEST_F(Bindings, test_delete_runtime_property_host_in_permanent_binding_group)
{
    auto target = std::make_shared<MetaBase>();
    auto runtime = target->setProperty(RuntimePropertyType, 10);
    WritableTest source(20);
    auto group = BindingGroup::bindPermanent(*runtime, source.writable);
    EXPECT_NOT_NULL(group);
    EXPECT_EQ(1u, group->getBindingCount());

    target.reset();
    EXPECT_FALSE(group->isEmpty());
    EXPECT_EQ(BindingState::Detached, group->at(0u)->getState());
}

TEST_F(Bindings, test_delete_runtime_property_host_in_autodetach_binding_group)
{
    auto target = std::make_shared<MetaBase>();
    auto runtime = target->setProperty(RuntimePropertyType, 10);
    WritableTest source(20);
    auto group = BindingGroup::bind(*runtime, source.writable);
    EXPECT_NOT_NULL(group);
    EXPECT_EQ(1u, group->getBindingCount());

    target.reset();
    EXPECT_FALSE(group->isEmpty());
    EXPECT_EQ(BindingState::Detached, group->at(0u)->getState());
}

TEST_F(Bindings, test_delete_runtime_property_host_in_permanent_cyclic_binding_group)
{
    auto target = std::make_shared<MetaBase>();
    auto runtime = target->setProperty(RuntimePropertyType, 10);
    WritableTest source(20);
    auto group = BindingGroup::bindPermanentCircular(*runtime, source.writable);
    EXPECT_NOT_NULL(group);
    EXPECT_EQ(2u, group->getBindingCount());

    target.reset();
    EXPECT_FALSE(group->isEmpty());
    EXPECT_EQ(BindingState::Detached, group->at(0u)->getState());
    EXPECT_EQ(BindingState::Detached, group->at(1u)->getState());
}

TEST_F(Bindings, test_delete_runtime_property_host_in_autodetach_cyclic_binding_group)
{
    auto target = std::make_shared<MetaBase>();
    auto runtime = target->setProperty(RuntimePropertyType, 10);
    WritableTest source(20);
    auto group = BindingGroup::bindCircular(*runtime, source.writable);
    EXPECT_NOT_NULL(group);
    EXPECT_EQ(2u, group->getBindingCount());

    target.reset();
    EXPECT_FALSE(group->isEmpty());
    EXPECT_EQ(BindingState::Detached, group->at(0u)->getState());
    EXPECT_EQ(BindingState::Detached, group->at(1u)->getState());
}
