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

#include <mox/core/meta/class/metaclass.hpp>
#include <mox/core/meta/class/metaobject.hpp>
#include "test_framework.h"

using namespace mox;

class TBaseClass
{
public:
    MetaInfo(TBaseClass)
    {
    };

    explicit TBaseClass()
    {
    }
    virtual ~TBaseClass() = default;
};

class BaseObject : public MetaObject, public TBaseClass
{
public:
    MetaInfo(BaseObject, MetaObject, TBaseClass)
    {
    };
};

class OtherBaseClass
{
public:
    MetaInfo(OtherBaseClass)
    {
    };

    explicit OtherBaseClass()
    {
    }
    virtual ~OtherBaseClass() = default;
};

class DerivedClass : public TBaseClass, public OtherBaseClass
{
public:
    explicit DerivedClass()
    {
    }

    MetaInfo(DerivedClass, TBaseClass, OtherBaseClass)
    {
    };
};

class ObjectDerivedClass : public MetaObject, public DerivedClass
{
public:
    MetaInfo(ObjectDerivedClass, MetaObject, DerivedClass)
    {
    };
};

class SecondLevelDerived : public DerivedClass
{
public:
    explicit SecondLevelDerived()
    {
    }

    virtual void noop() = 0;

    MetaInfo(SecondLevelDerived, DerivedClass)
    {
    };
};

class SecondObject : public MetaObject, public SecondLevelDerived
{
public:
    MetaInfo(SecondObject, MetaObject, SecondLevelDerived)
    {
    };
    void noop() override
    {
    }
};

class MetaClasses : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaClass<TBaseClass>();
        registerMetaClass<BaseObject>();
        registerMetaClass<OtherBaseClass>();
        registerMetaClass<DerivedClass>();
        registerMetaClass<ObjectDerivedClass>();
        registerMetaClass<SecondLevelDerived>();
        registerMetaClass<SecondObject>();
    }
};

TEST_F(MetaClasses, test_metaclass_ownership)
{
    const auto* mo = TBaseClass::StaticMetaClass::get();
    BaseObject object;
    EXPECT_TRUE(mo->isClassOf(object));
}

TEST_F(MetaClasses, test_composit_interface_metaclass)
{
    const auto* moBaseClass = TBaseClass::StaticMetaClass::get();
    const auto* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const auto* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const auto* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    ObjectDerivedClass object;

    EXPECT_TRUE(moBaseClass->isClassOf(object));
    EXPECT_TRUE(moOtherBaseClass->isClassOf(object));
    EXPECT_TRUE(moDerivedClass->isClassOf(object));
    EXPECT_TRUE(moObjectDerivedClass->isClassOf(object));
}

TEST_F(MetaClasses, test_superclass)
{
    const auto* moBaseClass = TBaseClass::StaticMetaClass::get();
    const auto* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const auto* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const auto* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    const auto* moMetaObject = MetaObject::StaticMetaClass::get();

    EXPECT_FALSE(moObjectDerivedClass->isSuperClassOf(*moMetaObject));
    EXPECT_TRUE(moObjectDerivedClass->derivesFrom(*moMetaObject));
    EXPECT_TRUE(moMetaObject->isSuperClassOf(*moObjectDerivedClass));
    EXPECT_TRUE(moBaseClass->isSuperClassOf(*moObjectDerivedClass));
    EXPECT_TRUE(moBaseClass->isSuperClassOf(*moDerivedClass));
    EXPECT_TRUE(moOtherBaseClass->isSuperClassOf(*moObjectDerivedClass));
    EXPECT_TRUE(moDerivedClass->isSuperClassOf(*moObjectDerivedClass));
    EXPECT_FALSE(moBaseClass->isSuperClassOf(*moOtherBaseClass));
}

TEST_F(MetaClasses, test_abstract)
{
    EXPECT_TRUE(SecondLevelDerived::StaticMetaClass::get()->isAbstract());
    EXPECT_FALSE(DerivedClass::StaticMetaClass::get()->isAbstract());
}

TEST_F(MetaClasses, test_second_object)
{
    ObjectDerivedClass o1;
    SecondObject o2;

    const auto* moBaseClass = TBaseClass::StaticMetaClass::get();
    const auto* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const auto* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const auto* moSecondLevelDerived = SecondLevelDerived::StaticMetaClass::get();
    const auto* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    const auto* moMetaObject = MetaObject::StaticMetaClass::get();

    EXPECT_TRUE(moBaseClass->isClassOf(o1));
    EXPECT_TRUE(moBaseClass->isClassOf(o2));

    EXPECT_TRUE(moOtherBaseClass->isClassOf(o1));
    EXPECT_TRUE(moOtherBaseClass->isClassOf(o2));

    EXPECT_TRUE(moDerivedClass->isClassOf(o1));
    EXPECT_TRUE(moDerivedClass->isClassOf(o2));

    EXPECT_FALSE(moSecondLevelDerived->isClassOf(o1));
    EXPECT_TRUE(moSecondLevelDerived->isClassOf(o2));

    EXPECT_TRUE(moObjectDerivedClass->isClassOf(o1));
    EXPECT_FALSE(moObjectDerivedClass->isClassOf(o2));

    EXPECT_TRUE(moMetaObject->isClassOf(o1));
    EXPECT_TRUE(moMetaObject->isClassOf(o2));
}

TEST_F(MetaClasses, test_find)
{
    EXPECT_TRUE(nullptr != metainfo::MetaClass::find("TBaseClass"));
    EXPECT_TRUE(nullptr == metainfo::MetaClass::find("Boo"));
    EXPECT_TRUE(nullptr == metainfo::MetaClass::find("baseClass"));
}

TEST_F(MetaClasses, test_metatype_superclass)
{
    SecondObject::StaticMetaClass::get();
    const auto& base = metatypeDescriptor<TBaseClass>();
    const auto& derived = metatypeDescriptor<SecondLevelDerived>();
    const auto& metaObject = metatypeDescriptor<MetaObject>();
    const auto& secondObject = metatypeDescriptor<SecondObject>();

    EXPECT_TRUE(derived.derivesFrom(base));
    EXPECT_TRUE(base.isSupertypeOf(derived));
    EXPECT_FALSE(metaObject.isSupertypeOf(base));
    EXPECT_FALSE(metaObject.derivesFrom(base));
    EXPECT_FALSE(metaObject.isSupertypeOf(derived));
    EXPECT_FALSE(metaObject.derivesFrom(derived));

    EXPECT_TRUE(secondObject.derivesFrom(base));
    EXPECT_TRUE(secondObject.derivesFrom(derived));
    EXPECT_TRUE(secondObject.derivesFrom(metaObject));
}
