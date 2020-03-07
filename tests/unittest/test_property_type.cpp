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

#include <test_framework.h>
#include <mox/config/deftypes.hpp>
#include <mox/utils/locks.hpp>
#include <mox/meta/property/property_type.hpp>

using namespace mox;

using ISPair = std::pair<int, std::string>;

class PropertyTypeTest : public ::UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaType<ISPair>("pair<int, string>");
    }
};

template <typename T>
class TestDataProvider : public PropertyData<T>
{
public:
    TestDataProvider(const T& value = T())
        : PropertyData<T>(value)
    {
    }

    void update(const T& value)
    {
        PropertyData<T>::update(Variant(value));
    }
};

static inline TestDataProvider<int> StatusData;

static inline SignalTypeDecl<int> IntChangedSignalType;
static inline SignalTypeDecl<std::string> StringChangedSignalType;
static inline SignalTypeDecl<int> StatusChangedSignalType;
static inline SignalTypeDecl<ISPair> ISPairChangedSignalType;

static inline PropertyTypeDecl<int, PropertyAccess::ReadWrite> IntPropertyType = {IntChangedSignalType, -1};
static inline PropertyTypeDecl<std::string, PropertyAccess::ReadWrite> StringPropertyType = {StringChangedSignalType, "coke"s};
static inline PropertyTypeDecl<int, PropertyAccess::ReadOnly> StatusPropertyType = {StatusChangedSignalType, -1};
static inline PropertyTypeDecl<ISPair, PropertyAccess::ReadWrite> ISPairPropertyType = {ISPairChangedSignalType};

TEST_F(PropertyTypeTest, test_proeprty_type_api)
{
    EXPECT_EQ(Metatype::Int32, IntPropertyType.getValueType().getType());
    EXPECT_FALSE(IntPropertyType.getValueType().isConst());
    EXPECT_FALSE(IntPropertyType.getValueType().isReference());
    EXPECT_EQ(-1, int(IntPropertyType.getDefault()));
    EXPECT_EQ(PropertyAccess::ReadWrite, IntPropertyType.getAccess());
    EXPECT_EQ(1u, IntPropertyType.ChangedSignalType.getArguments().size());
    EXPECT_EQ(Metatype::Int32, IntPropertyType.ChangedSignalType.getArguments()[0].getType());
    EXPECT_FALSE(IntPropertyType.ChangedSignalType.getArguments()[0].isConst());
    EXPECT_FALSE(IntPropertyType.ChangedSignalType.getArguments()[0].isReference());
}

TEST_F(PropertyTypeTest, test_simple_property_type)
{
    EXPECT_EQ(Metatype::Int32, IntPropertyType.getValueType().getType());
    EXPECT_EQ(Metatype::String, StringPropertyType.getValueType().getType());
    EXPECT_EQ(Metatype::Int32, StatusPropertyType.getValueType().getType());

    EXPECT_EQ(PropertyAccess::ReadWrite, IntPropertyType.getAccess());
    EXPECT_EQ(PropertyAccess::ReadWrite, StringPropertyType.getAccess());
    EXPECT_EQ(PropertyAccess::ReadOnly, StatusPropertyType.getAccess());
}

TEST_F(PropertyTypeTest, test_complex_property_type)
{
    MetaBase host;
    PropertyData<ISPair> data(std::make_pair(1, "foo"s));
    Property property(host, ISPairPropertyType, data);

    EXPECT_EQ(1, ISPair(property).first);
    EXPECT_EQ("foo"s, ISPair(property).second);

    int count = 0;
    auto onChanged = [&count]() { count++; };
    property.changed.connect(onChanged);

    property = std::make_pair(2, "bar"s);
    EXPECT_EQ(1, count);
    EXPECT_EQ(2, ISPair(property).first);
    EXPECT_EQ("bar"s, ISPair(property).second);
}
