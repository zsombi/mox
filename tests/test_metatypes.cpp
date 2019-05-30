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
#include <mox/metadata/metatype_descriptor.hpp>

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
        registerMetaType<UserStruct>();
        registerMetaType<UserClass>();
    }
};

TEST_F(Types, test_atomic_types)
{
    const MetatypeDescriptor* type = &metatypeDescriptor<bool>();
    EXPECT_EQ(Metatype::Bool, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "bool"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<char>();
    EXPECT_EQ(Metatype::Char, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "char"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<byte>();
    EXPECT_EQ(Metatype::Byte, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "byte"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    // in c++11 byte is an enum class!!
    EXPECT_TRUE(type->isEnum());

    type = &metatypeDescriptor<short>();
    EXPECT_EQ(Metatype::Short, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "short"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<unsigned short>();
    EXPECT_EQ(Metatype::Word, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "word"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<int>();
    EXPECT_EQ(Metatype::Int, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<unsigned int>();
    EXPECT_EQ(Metatype::UInt, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<long>();
    EXPECT_EQ(Metatype::Long, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "long"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<unsigned long>();
    EXPECT_EQ(Metatype::ULong, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "ulong"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<long long>();
    EXPECT_EQ(Metatype::Int64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "int64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<unsigned long long>();
    EXPECT_EQ(Metatype::UInt64, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "uint64"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<float>();
    EXPECT_EQ(Metatype::Float, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "float"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<double>();
    EXPECT_EQ(Metatype::Double, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "double"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<void>();
    EXPECT_EQ(Metatype::Void, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "void"));
    EXPECT_TRUE(type->isValid());
    EXPECT_TRUE(type->isVoid());
    EXPECT_FALSE(type->isEnum());

    type = &metatypeDescriptor<std::string>();
    EXPECT_EQ(Metatype::String, type->id());
    EXPECT_TRUE(!strcmp(type->name(), "std::string"));
    EXPECT_TRUE(type->isValid());
    EXPECT_FALSE(type->isVoid());
    EXPECT_FALSE(type->isEnum());
}

TEST_F(Types, test_synonim_types)
{
    const MetatypeDescriptor* type;

    type = &metatypeDescriptor<int32_t>();
    EXPECT_EQ(Metatype::Int, type->id());

    type = &metatypeDescriptor<int16_t>();
    EXPECT_EQ(Metatype::Short, type->id());

    type = &metatypeDescriptor<intptr_t>();
    EXPECT_EQ(Metatype::Long, type->id());

    type = &metatypeDescriptor<size_t>();
    EXPECT_EQ(Metatype::ULong, type->id());
}

TEST_F(Types, test_composit_types)
{
    const MetatypeDescriptor* type;

    type = &metatypeDescriptor<int*>();
    EXPECT_EQ(Metatype::Int, type->id());

    type = &metatypeDescriptor<int&>();
    EXPECT_EQ(Metatype::Int, type->id());

    type = &metatypeDescriptor<const int*>();
    EXPECT_EQ(Metatype::Int, type->id());

    type = &metatypeDescriptor<const int&>();
    EXPECT_EQ(Metatype::Int, type->id());
}

TEST_F(Types, test_user_types)
{
    const MetatypeDescriptor* type;

    type = &metatypeDescriptor<UserStruct>();
    EXPECT_GE(type->id(), Metatype::UserType);
    EXPECT_TRUE(type->isClass());

    type = &metatypeDescriptor<UserClass>();
    EXPECT_GE(type->id(), Metatype::UserType);

    EXPECT_EQ(metaType<UserClass>(), metaType<UserClass*>());
}
