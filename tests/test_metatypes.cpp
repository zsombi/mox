/*
 * Copyright (C) 2017-2018 bitWelder
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
#include <mox/metadata/metatype.hpp>

using namespace mox;

struct UserStruct
{
};

class UserClass
{
};

class Types : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        MetaType::registerMetaType<UserStruct>();
        MetaType::registerMetaType<UserClass>();
    }
};

TEST_F(Types, test_atomic_types)
{
    const MetaType* type = &MetaType::get<bool>();
    EXPECT_EQ(MetaType::TypeId::Bool, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "bool"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<char>();
    EXPECT_EQ(MetaType::TypeId::Char, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "char"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<byte>();
    EXPECT_EQ(MetaType::TypeId::Byte, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "byte"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    // in c++11 byte is an enum class!!
    EXPECT_TRUE(type->isEnum());

    type = &MetaType::get<short>();
    EXPECT_EQ(MetaType::TypeId::Short, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "short"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<unsigned short>();
    EXPECT_EQ(MetaType::TypeId::Word, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "word"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<int>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<unsigned int>();
    EXPECT_EQ(MetaType::TypeId::UInt, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<long>();
    EXPECT_EQ(MetaType::TypeId::Long, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "long"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<unsigned long>();
    EXPECT_EQ(MetaType::TypeId::ULong, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "ulong"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<long long>();
    EXPECT_EQ(MetaType::TypeId::Int64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<unsigned long long>();
    EXPECT_EQ(MetaType::TypeId::UInt64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<float>();
    EXPECT_EQ(MetaType::TypeId::Float, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "float"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<double>();
    EXPECT_EQ(MetaType::TypeId::Double, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "double"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<void>();
    EXPECT_EQ(MetaType::TypeId::Void, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "void"));
    EXPECT_TRUE(type->isValid());
    EXPECT_TRUE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaType::get<std::string>();
    EXPECT_EQ(MetaType::TypeId::StdString, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "std::string"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());
}

TEST_F(Types, test_synonim_types)
{
    const MetaType* type;

    type = &MetaType::get<int32_t>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());

    type = &MetaType::get<int16_t>();
    EXPECT_EQ(MetaType::TypeId::Short, type->id());

    type = &MetaType::get<intptr_t>();
    EXPECT_EQ(MetaType::TypeId::Long, type->id());

    type = &MetaType::get<size_t>();
    EXPECT_EQ(MetaType::TypeId::ULong, type->id());
}

TEST_F(Types, test_composit_types)
{
    const MetaType* type;

    type = &MetaType::get<int*>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());

    type = &MetaType::get<int&>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());

    type = &MetaType::get<const int*>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());

    type = &MetaType::get<const int&>();
    EXPECT_EQ(MetaType::TypeId::Int, type->id());
}

TEST_F(Types, test_user_types)
{
    const MetaType* type;

    type = &MetaType::get<UserStruct>();
    EXPECT_GE(type->id(), MetaType::TypeId::UserType);

    type = &MetaType::get<UserClass>();
    EXPECT_GE(type->id(), MetaType::TypeId::UserType);

    EXPECT_EQ(MetaType::typeId<UserClass>(), MetaType::typeId<UserClass*>());
}
