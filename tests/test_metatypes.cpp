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
        MetaTypeDescriptor::registerMetaType<UserStruct>();
        MetaTypeDescriptor::registerMetaType<UserClass>();
    }
};

TEST_F(Types, test_atomic_types)
{
    const MetaTypeDescriptor* type = &MetaTypeDescriptor::get<bool>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Bool, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "bool"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<char>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Char, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "char"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<byte>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Byte, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "byte"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    // in c++11 byte is an enum class!!
    EXPECT_TRUE(type->isEnum());

    type = &MetaTypeDescriptor::get<short>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Short, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "short"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<unsigned short>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Word, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "word"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<int>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<unsigned int>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::UInt, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<long>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Long, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "long"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<unsigned long>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::ULong, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "ulong"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<long long>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<unsigned long long>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::UInt64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<float>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Float, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "float"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<double>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Double, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "double"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<void>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Void, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "void"));
    EXPECT_TRUE(type->isValid());
    EXPECT_TRUE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &MetaTypeDescriptor::get<std::string>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::String, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "std::string"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());
}

TEST_F(Types, test_synonim_types)
{
    const MetaTypeDescriptor* type;

    type = &MetaTypeDescriptor::get<int32_t>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());

    type = &MetaTypeDescriptor::get<int16_t>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Short, type->id());

    type = &MetaTypeDescriptor::get<intptr_t>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Long, type->id());

    type = &MetaTypeDescriptor::get<size_t>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::ULong, type->id());
}

TEST_F(Types, test_composit_types)
{
    const MetaTypeDescriptor* type;

    type = &MetaTypeDescriptor::get<int*>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());

    type = &MetaTypeDescriptor::get<int&>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());

    type = &MetaTypeDescriptor::get<const int*>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());

    type = &MetaTypeDescriptor::get<const int&>();
    EXPECT_EQ(MetaTypeDescriptor::TypeId::Int, type->id());
}

TEST_F(Types, test_user_types)
{
    const MetaTypeDescriptor* type;

    type = &MetaTypeDescriptor::get<UserStruct>();
    EXPECT_GE(type->id(), MetaTypeDescriptor::TypeId::UserType);
    EXPECT_TRUE(type->isClass());

    type = &MetaTypeDescriptor::get<UserClass>();
    EXPECT_GE(type->id(), MetaTypeDescriptor::TypeId::UserType);

    EXPECT_EQ(MetaTypeDescriptor::typeId<UserClass>(), MetaTypeDescriptor::typeId<UserClass*>());
}
