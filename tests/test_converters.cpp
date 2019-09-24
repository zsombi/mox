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

#include <mox/metadata/variant.hpp>
#include <mox/metadata/metaobject.hpp>

namespace converter_test
{

struct UserType
{
    int16_t v1;
    int16_t v2;
    explicit UserType()
        : v1(0), v2(0)
    {}
};

struct SelfConvertible
{
    int16_t v1 = 0l;
    int16_t v2 = 0l;

    UserType convert() const
    {
        return *reinterpret_cast<const UserType*>(this);
    }
    UserType* ptr_convert() const
    {
        return const_cast<UserType*>(reinterpret_cast<const UserType*>(this));
    }
};

UserType convert(int32_t v)
{
    UserType ret;
    memcpy(&ret, &v, sizeof(ret));
    return ret;
}

int32_t convert2(UserType v)
{
    int32_t ret;
    memcpy(&ret, &v, sizeof(ret));
    return ret;
}

class Derived : public mox::MetaObject
{
public:
    explicit Derived() = default;

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, Derived, mox::MetaObject>
    {
    };
};

} // converter_test

class Converters : public UnitTest
{
protected:
    void SetUp() override
    {
        static_assert (sizeof(int32_t) == sizeof(converter_test::UserType), "revisit UserType declaration");
        static_assert (sizeof(converter_test::SelfConvertible) == sizeof(converter_test::UserType), "revisit SelfConvertible declaration");
        UnitTest::SetUp();

        mox::registerMetaType<converter_test::UserType>();
        mox::registerMetaType<converter_test::UserType*>();
        mox::registerMetaType<converter_test::SelfConvertible>();
        mox::registerMetaClass<converter_test::Derived>();
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
                mox::MetatypeConverter* converter = mox::metadata::findConverter(from, to);
                EXPECT_NOT_NULL(converter);
            }
        }
    }
}

TEST_F(Converters, test_register_converter_function)
{
    bool b = mox::registerConverter<int32_t, converter_test::UserType>(converter_test::convert);
    EXPECT_TRUE(b);

    // Convert long long to UserType.
    mox::MetatypeConverter* converter = mox::metadata::findConverter(mox::Metatype::Int32, mox::metaType<converter_test::UserType>());
    EXPECT_NOT_NULL(converter);

    converter_test::UserType result;
    int32_t v = 65537;
    mox::MetaValue r = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<converter_test::UserType>(r));
    EXPECT_EQ(1, result.v1);
    EXPECT_EQ(1, result.v2);

    converter = mox::metadata::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int32);
    EXPECT_NULL(converter);
}

TEST_F(Converters, test_register_converter_functor)
{
    auto userType2int32 = [](converter_test::UserType value) -> int32_t
    {
        int32_t l = 0;
        memcpy((byte*)&l, (byte*)&value, sizeof(l));
        return l;
    };
    bool b = mox::registerConverter<converter_test::UserType, int32_t>(userType2int32);
    EXPECT_TRUE(b);

    auto converter = mox::metadata::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int32);
    EXPECT_NOT_NULL(converter);

    converter_test::UserType v;
    v.v1 = 1;
    v.v2 = 1;
    int32_t result = 0;
    mox::MetaValue presult = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<int32_t>(presult));
    EXPECT_EQ(65537, result);
}

TEST_F(Converters, test_registered_functor_converter)
{
    auto converter = mox::metadata::findConverter(mox::metaType<converter_test::UserType>(), mox::Metatype::Int32);
    EXPECT_NOT_NULL(converter);

    converter_test::UserType v;
    v.v1 = 1;
    v.v2 = 1;
    int32_t result = 0;
    mox::MetaValue presult = converter->convert(*converter, &v);
    EXPECT_NO_THROW(result = std::any_cast<int32_t>(presult));
    EXPECT_EQ(65537, result);
}

TEST_F(Converters, test_register_converter_method)
{
    bool b = mox::registerConverter<converter_test::SelfConvertible, converter_test::UserType>(&converter_test::SelfConvertible::convert);
    EXPECT_TRUE(b);
    b = mox::registerConverter<converter_test::SelfConvertible, converter_test::UserType*>(&converter_test::SelfConvertible::ptr_convert);
    EXPECT_TRUE(b);

    mox::MetatypeConverter* converter = mox::metadata::findConverter(mox::metaType<converter_test::SelfConvertible>(), mox::metaType<converter_test::UserType>());
    EXPECT_NOT_NULL(converter);

    converter_test::SelfConvertible src;
    src.v1 = 10;
    src.v2 = 20;

    converter_test::UserType dst;
    mox::MetaValue anyresult = converter->convert(*converter, &src);
    EXPECT_NO_THROW(dst = std::any_cast<converter_test::UserType>(anyresult));
    EXPECT_EQ(10, dst.v1);
    EXPECT_EQ(20, dst.v2);

    converter = mox::metadata::findConverter(mox::metaType<converter_test::SelfConvertible>(), mox::metaType<converter_test::UserType*>());
    EXPECT_NOT_NULL(converter);

    converter_test::UserType* pdst = nullptr;
    anyresult = converter->convert(*converter, &src);
    EXPECT_NO_THROW(pdst = std::any_cast<converter_test::UserType*>(anyresult));
    EXPECT_EQ(10, pdst->v1);
    EXPECT_EQ(20, pdst->v2);
}

TEST_F(Converters, test_metaobject_conversion)
{
    mox::Variant arg;
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
