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
#include <mox/metainfo/metaclass.hpp>
#include <mox/metatype.core/metadata.hpp>
#include <mox/metatype.core/metatype.hpp>
#include <mox/metatype.core/metatype_descriptor.hpp>

TEST(MetaDataEnum, test_enumerate_default_metatypes)
{
    // Test the predefined types.
    std::array<std::string, int(mox::Metatype::UserType)> typeNames = {
        "void"s, "bool"s, "char"s, "byte"s, "short"s, "word"s, "int"s, "uint"s, "int64"s, "uint64"s,
        "float"s, "double"s, "std::string"s, "literal"s, "void*"s, "byte*"s, "int*"s, "int64*"s,
        "vector<int32>"s
    };
    auto it = typeNames.begin();
    auto scanner = [&it, &typeNames](const auto& des)
    {
        EXPECT_EQ(*it, des.name());
        ++it;
        return it == typeNames.end();
    };
    mox::metadata::findMetatype(scanner);
}

TEST(MetaDataEnum, test_find_user_metatypes)
{
    std::array<std::string, 8> typeNames = {
        "mox::Object"s, "mox::Object*"s, "mox::MetaObject"s, "mox::MetaObject*"s,
        "mox::MetaBase"s, "mox::MetaBase*"s, "mox::ThreadLoop"s, "mox::ThreadLoop*"s,
    };
    for (auto name : typeNames)
    {
        auto scanner = [&name](const auto& des)
        {
            return name == des.name();
        };
        auto typeDes = mox::metadata::findMetatype(scanner);
        EXPECT_NOT_NULL(typeDes);
        EXPECT_EQ(name, typeDes->name());
    }
}

struct MetaTest
{
    std::string metaClass;
    std::vector<std::string> properties;
    std::vector<std::string> signals;
    std::vector<std::string> methods;

    void clear()
    {
        metaClass.clear();
        properties.clear();
        signals.clear();
        methods.clear();
    }

    bool verifyMetaClass(const mox::metainfo::MetaClass& mc)
    {
        bool result = metaClass == mox::MetatypeDescriptor::get(mc.getMetaTypes().first).name();

        if (!result)
        {
            return result;
        }

        auto propertyIt = properties.begin();
        auto propertyVisitor = [this, &propertyIt](const auto, const auto& meta)
        {
            EXPECT_EQ(*propertyIt, meta.name());
            ++propertyIt;
            return propertyIt == properties.end();
        };
        mc.visitProperties(propertyVisitor);

        auto signalIt = signals.begin();
        auto signalVisitor = [&signalIt, this](const auto, const auto& meta)
        {
            EXPECT_EQ(*signalIt, meta.name());
            ++signalIt;
            return signalIt == signals.end();
        };
        mc.visitSignals(signalVisitor);

        auto methodIt = methods.begin();
        auto methodVisitor = [&methodIt, this](const auto, const auto& meta)
        {
            EXPECT_EQ(*methodIt, meta.name());
            ++methodIt;
            return methodIt == methods.end();
        };
        mc.visitMethods(methodVisitor);
        return result;
    }
};

TEST(MetaDataEnum, test_enumerate_metaclasses)
{
    MetaTest test;
    test.metaClass = "mox::Object";
    test.properties.push_back("objectName"s);
    test.signals.push_back("objectNameChanged");

    EXPECT_NOT_NULL(mox::metainfo::find(std::bind(&MetaTest::verifyMetaClass, &test, std::placeholders::_1)));
    test.clear();

    test.metaClass = "mox::ThreadLoop";
    test.properties.push_back("objectName"s);
    test.signals.push_back("started");
    test.signals.push_back("stopped");
    test.signals.push_back("objectNameChanged");

    EXPECT_NOT_NULL(mox::metainfo::find(std::bind(&MetaTest::verifyMetaClass, &test, std::placeholders::_1)));
    test.clear();

    test.metaClass = "mox::Application";
    test.signals.push_back("started");
    test.signals.push_back("stopped");
    test.methods.push_back("quit");

    EXPECT_NOT_NULL(mox::metainfo::find(std::bind(&MetaTest::verifyMetaClass, &test, std::placeholders::_1)));
    test.clear();
}
