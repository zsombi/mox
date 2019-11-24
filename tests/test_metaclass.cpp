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

#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metaobject.hpp>
#include "test_framework.h"

using namespace mox;

class TBaseClass
{
public:
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, TBaseClass>
    {
        MetaClassDefs()
    };

    explicit TBaseClass()
    {
    }
    virtual ~TBaseClass() = default;
};

class BaseObject : public MetaObject, public TBaseClass
{
public:
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, BaseObject, MetaObject, TBaseClass>
    {
        MetaClassDefs()
    };
};

class OtherBaseClass
{
public:
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, OtherBaseClass>
    {
        MetaClassDefs()
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

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, DerivedClass, TBaseClass, OtherBaseClass>
    {
        MetaClassDefs()
    };
};

class ObjectDerivedClass : public MetaObject, public DerivedClass
{
public:
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, ObjectDerivedClass, MetaObject, DerivedClass>
    {
        MetaClassDefs()
    };
};

class SecondLevelDerived : public DerivedClass
{
public:
    explicit SecondLevelDerived()
    {
    }

    virtual void noop() = 0;

    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, SecondLevelDerived, DerivedClass>
    {
        MetaClassDefs()
    };
};

class SecondObject : public MetaObject, public SecondLevelDerived
{
public:
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, SecondObject, MetaObject, SecondLevelDerived>
    {
        MetaClassDefs()
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
    const MetaClass* mo = TBaseClass::StaticMetaClass::get();
    BaseObject object;
    EXPECT_TRUE(mo->isClassOf(object));
}

TEST_F(MetaClasses, test_composit_interface_metaclass)
{
    const MetaClass* moBaseClass = TBaseClass::StaticMetaClass::get();
    const MetaClass* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const MetaClass* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    ObjectDerivedClass object;

    EXPECT_TRUE(moBaseClass->isClassOf(object));
    EXPECT_TRUE(moOtherBaseClass->isClassOf(object));
    EXPECT_TRUE(moDerivedClass->isClassOf(object));
    EXPECT_TRUE(moObjectDerivedClass->isClassOf(object));
}

TEST_F(MetaClasses, test_superclass)
{
    const MetaClass* moBaseClass = TBaseClass::StaticMetaClass::get();
    const MetaClass* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const MetaClass* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    const MetaClass* moMetaObject = MetaObject::StaticMetaClass::get();

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

    const MetaClass* moBaseClass = TBaseClass::StaticMetaClass::get();
    const MetaClass* moOtherBaseClass = OtherBaseClass::StaticMetaClass::get();
    const MetaClass* moDerivedClass = DerivedClass::StaticMetaClass::get();
    const MetaClass* moSecondLevelDerived = SecondLevelDerived::StaticMetaClass::get();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::StaticMetaClass::get();
    const MetaClass* moMetaObject = MetaObject::StaticMetaClass::get();

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
    EXPECT_TRUE(nullptr != MetaClass::find("TBaseClass"));
    EXPECT_TRUE(nullptr == MetaClass::find("Boo"));
    EXPECT_TRUE(nullptr == MetaClass::find("baseClass"));
}

TEST_F(MetaClasses, test_metatype_superclass)
{
    SecondObject::StaticMetaClass::get();
    const MetatypeDescriptor& base = metatypeDescriptor<TBaseClass>();
    const MetatypeDescriptor& derived = metatypeDescriptor<SecondLevelDerived>();
    const MetatypeDescriptor& metaObject = metatypeDescriptor<MetaObject>();
    const MetatypeDescriptor& secondObject = metatypeDescriptor<SecondObject>();

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
