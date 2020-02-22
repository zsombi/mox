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
#include <mox/object.hpp>

using namespace mox;


TEST(ObjectTest, test_api)
{
    ObjectSharedPtr object = Object::create();

    EXPECT_NULL(object->parent());
    EXPECT_EQ(0u, object->childCount());
}

TEST(ObjectTest, test_add_child)
{
    ObjectSharedPtr parent = Object::create();
    ObjectSharedPtr child1 = Object::create(parent.get());

    EXPECT_NULL(parent->parent());
    EXPECT_EQ(1u, parent->childCount());
    EXPECT_EQ(child1, parent->childAt(0));
    EXPECT_EQ(parent.get(), child1->parent());
}

TEST(ObjectTest, test_remove_child)
{
    ObjectSharedPtr parent = Object::create();
    ObjectSharedPtr child1 = Object::create(parent.get());
    ObjectSharedPtr child2 = Object::create(parent.get());
    ObjectSharedPtr child11 = Object::create(child1.get());

    EXPECT_EQ(2u, parent->childCount());
    EXPECT_EQ(1u, child1->childCount());
    EXPECT_EQ(child1.get(), child11->parent());
    parent->removeChild(*child1);
    EXPECT_EQ(1u, parent->childCount());
}
