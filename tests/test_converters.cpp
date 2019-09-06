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

#include <mox/metadata/argument.hpp>
#include <mox/metadata/metaobject.hpp>

namespace converter_test
{

struct UserType
{
    Int16 v1;
    Int16 v2;
    explicit UserType()
        : v1(0), v2(0)
    {}
};

struct SelfConvertible
{
    Int16 v1 = 0l;
    Int16 v2 = 0l;

    UserType convert() const
    {
        return *reinterpret_cast<const UserType*>(this);
    }
    UserType* ptr_convert() const
    {
        return const_cast<UserType*>(reinterpret_cast<const UserType*>(this));
    }
};

UserType convert(Int32 v)
{
    UserType ret;
    memcpy(&ret, &v, sizeof(ret));
    return ret;
}

Int32 convert2(UserType v)
{
    Int32 ret;
    memcpy(&ret, &v, sizeof(ret));
    return ret;
}

class Derived : public mox::MetaObject
{
public:
    explicit Derived() = default;

    METACLASS(Derived, mox::MetaObject)
    {
    };
};

} // converter_test

class Converters : public UnitTest
{
protected:
    void SetUp() override
    {
        static_assert (sizeof(Int32) == sizeof(converter_test::UserType), "revisit UserType declaration");
        static_assert (sizeof(converter_test::SelfConvertible) == sizeof(converter_test::UserType), "revisit SelfConvertible declaration");
        UnitTest::SetUp();

        registerTestType<converter_test::UserType>();
        registerTestType<converter_test::SelfConvertible>();
        registerTestType<converter_test::Derived>();
    }
};

TEST_F(Converters, test_base_type_converters)
{
    for (mox::Metatype from = mox::Metatype::Bool; from < mox::Metatype::NumericMax; ++from)
    {
        for (mox::Metatype to = mox::Metatype::Bool; to < mox::Metatype::NumericMax; ++to)
        {
            if (from != to)
            {
                mox::MetatypeConverter* converter = mox::registrar::findConverter(from, to);
                EXPECT_NOT_NULL(converter);
            }
        }
    }
}

TEST_F(Converters, test_register_converter_function)
{
    bool b = mox::registerConverter<Int32, converter_test::UserType>(converter_test::convert);
    EXPECT_TRUE(b);

    // Convert long long to UserType.
    mox::MetatypeConverter* converter = mox::registrar::findConverter(mox::Metatype::Int, mox::metaType<converter_test::UserType>());
    EXPECT_NOT_NULL(converter);

    converter_test::UserType result;
    Int32 v = 65537;
    mox::ArgumentBase r = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<converter_test::UserType>(r));
    EXPECT_EQ(1, result.v1);
    EXPECT_EQ(1, result.v2);

    converter = mox::registrar::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int);
    EXPECT_NULL(converter);
}

TEST_F(Converters, test_register_converter_functor)
{
    auto userType2int32 = [](converter_test::UserType value) -> Int32
    {
        Int32 l = 0;
        memcpy((byte*)&l, (byte*)&value, sizeof(l));
        return l;
    };
    bool b = mox::registerConverter<converter_test::UserType, Int32>(userType2int32);
    EXPECT_TRUE(b);

    auto converter = mox::registrar::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int);
    EXPECT_NOT_NULL(converter);

    converter_test::UserType v;
    v.v1 = 1;
    v.v2 = 1;
    Int32 result = 0;
    mox::ArgumentBase presult = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<Int32>(presult));
    EXPECT_EQ(65537, result);
}

TEST_F(Converters, test_registered_functor_converter)
{
    auto converter = mox::registrar::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int);
    EXPECT_NOT_NULL(converter);

    converter_test::UserType v;
    v.v1 = 1;
    v.v2 = 1;
    Int32 result = 0;
    mox::ArgumentBase presult = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<Int32>(presult));
    EXPECT_EQ(65537, result);
}

TEST_F(Converters, test_register_converter_method)
{
    bool b = mox::registerConverter<converter_test::SelfConvertible, converter_test::UserType>(&converter_test::SelfConvertible::convert);
    EXPECT_TRUE(b);
    b = mox::registerConverter<converter_test::SelfConvertible, converter_test::UserType*>(&converter_test::SelfConvertible::ptr_convert);
    EXPECT_TRUE(b);

    mox::MetatypeConverter* converter = mox::registrar::findConverter(mox::metaType<converter_test::SelfConvertible>(), mox::metaType<converter_test::UserType>());
    EXPECT_NOT_NULL(converter);

    converter_test::SelfConvertible src;
    src.v1 = 10;
    src.v2 = 20;

    converter_test::UserType dst;
    mox::ArgumentBase anyresult = converter->convert(*converter, &src);
    EXPECT_NO_THROW(dst = std::any_cast<converter_test::UserType>(anyresult));
    EXPECT_EQ(10, dst.v1);
    EXPECT_EQ(20, dst.v2);

    converter = mox::registrar::findConverter(mox::metaType<converter_test::SelfConvertible>(), mox::metaType<converter_test::UserType*>());
    EXPECT_NOT_NULL(converter);

    converter_test::UserType* pdst = nullptr;
    anyresult = converter->convert(*converter, &src);
    EXPECT_NO_THROW(pdst = std::any_cast<converter_test::UserType*>(anyresult));
    EXPECT_EQ(10, pdst->v1);
    EXPECT_EQ(20, pdst->v2);
}

TEST_F(Converters, test_metaobject_conversion)
{
    mox::Argument arg;
    converter_test::Derived obj;
    arg = &obj;

    mox::MetaObject* pbase = arg;
    EXPECT_NOT_NULL(pbase);

    mox::MetaObject obj2;
    arg = &obj2;

    EXPECT_TRUE(arg.isValid());
    converter_test::Derived *pderived = arg;
    EXPECT_NULL(pderived);
}
