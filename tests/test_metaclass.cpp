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
#include "test_framework.h"

using namespace mox;

class BaseClass// : public MetaClassHelper<BaseClass>
{
public:
    MIXIN_METACLASS_BASE(BaseClass)
    {
    };

    explicit BaseClass()
    {
    }
};

class BaseObject : public MetaObject, public BaseClass
{
public:
    METACLASS(BaseObject, MetaObject, BaseClass)
    {
    };
};

class OtherBaseClass
{
public:
    MIXIN_METACLASS_BASE(OtherBaseClass)
    {
    };

    explicit OtherBaseClass()
    {
    }
};

class DerivedClass : public BaseClass, public OtherBaseClass
{
public:
    explicit DerivedClass()
    {
    }

    MIXIN_METACLASS(DerivedClass, BaseClass, OtherBaseClass)
    {
    };
};

class ObjectDerivedClass : public MetaObject, public DerivedClass
{
public:
    METACLASS(ObjectDerivedClass, MetaObject, DerivedClass)
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

    MIXIN_METACLASS(SecondLevelDerived, DerivedClass)
    {
    };
};

class SecondObject : public MetaObject, public SecondLevelDerived
{
public:
    METACLASS(SecondObject, MetaObject, SecondLevelDerived)
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
        MetaType::registerMetaType<BaseClass>();
        MetaType::registerMetaType<BaseObject>();
        MetaType::registerMetaType<OtherBaseClass>();
        MetaType::registerMetaType<DerivedClass>();
        MetaType::registerMetaType<ObjectDerivedClass>();
        MetaType::registerMetaType<SecondLevelDerived>();
        MetaType::registerMetaType<SecondObject>();
    }
};

TEST_F(MetaClasses, test_metaclass_ownership)
{
    const MetaClass* mo = BaseClass::getStaticMetaClass();
    BaseObject object;
    EXPECT_TRUE(mo->isClassOf(object));
}

TEST_F(MetaClasses, test_composit_interface_metaclass)
{
    const MetaClass* moBaseClass = BaseClass::getStaticMetaClass();
    const MetaClass* moOtherBaseClass = OtherBaseClass::getStaticMetaClass();
    const MetaClass* moDerivedClass = DerivedClass::getStaticMetaClass();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::getStaticMetaClass();
    ObjectDerivedClass object;

    EXPECT_TRUE(moBaseClass->isClassOf(object));
    EXPECT_TRUE(moOtherBaseClass->isClassOf(object));
    EXPECT_TRUE(moDerivedClass->isClassOf(object));
    EXPECT_TRUE(moObjectDerivedClass->isClassOf(object));
}

TEST_F(MetaClasses, test_superclass)
{
    const MetaClass* moBaseClass = BaseClass::getStaticMetaClass();
    const MetaClass* moOtherBaseClass = OtherBaseClass::getStaticMetaClass();
    const MetaClass* moDerivedClass = DerivedClass::getStaticMetaClass();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::getStaticMetaClass();
    const MetaClass* moMetaObject = MetaObject::getStaticMetaClass();

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
    EXPECT_TRUE(SecondLevelDerived::SecondLevelDerivedMetaClass::abstract);
    EXPECT_FALSE(DerivedClass::DerivedClassMetaClass::abstract);
    EXPECT_TRUE(SecondLevelDerived::getStaticMetaClass()->isAbstract());
    EXPECT_FALSE(DerivedClass::getStaticMetaClass()->isAbstract());
}

TEST_F(MetaClasses, test_second_object)
{
    ObjectDerivedClass o1;
    SecondObject o2;

    const MetaClass* moBaseClass = BaseClass::getStaticMetaClass();
    const MetaClass* moOtherBaseClass = OtherBaseClass::getStaticMetaClass();
    const MetaClass* moDerivedClass = DerivedClass::getStaticMetaClass();
    const MetaClass* moSecondLevelDerived = SecondLevelDerived::getStaticMetaClass();
    const MetaClass* moObjectDerivedClass = ObjectDerivedClass::getStaticMetaClass();
    const MetaClass* moMetaObject = MetaObject::getStaticMetaClass();

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
    EXPECT_TRUE(nullptr != MetaClass::find("BaseClass"));
    EXPECT_TRUE(nullptr == MetaClass::find("Boo"));
    EXPECT_TRUE(nullptr == MetaClass::find("baseClass"));
}

TEST_F(MetaClasses, test_metatype_superclass)
{
    SecondObject::getStaticMetaClass();
    const MetaType& base = MetaType::get<BaseClass>();
    const MetaType& derived = MetaType::get<SecondLevelDerived>();
    const MetaType& metaObject = MetaType::get<MetaObject>();
    const MetaType& secondObject = MetaType::get<SecondObject>();

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
