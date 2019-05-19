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

#ifndef METACLASS_HPP
#define METACLASS_HPP

#include <any>

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox
{

class MetaObject;
class MetaMethod;

/// MetaClass represents the type reflection or metadata of a managed structure or class. The metadata consists
/// of factory functions, methods, properties and signals.
///
struct MOX_API MetaClass
{
public:
    /// Method visitor function.
    typedef std::function<bool(const MetaMethod*)> MethodVisitor;

    /// Destructor.
    virtual ~MetaClass();

    /// Tests whether this MetaClass is the superclass of the \a metaClass passed as argument.
    bool isSuperClassOf(const MetaClass& metaClass) const;

    /// Returns the MetaClass that manages the \a className class.
    static const MetaClass* find(std::string_view className);

    /// Visits the metaclass passing the methods to the \a visitor.
    /// \param visitor The visitor.
    /// \return The MetaMethod instance for which the visitor returns \e true, \e nullptr if
    /// no method is identified by the visitor.
    const MetaMethod* visitMethods(const MethodVisitor& visitor) const;

    /// Returns the MetaType of the MetaClass.
    MetaType::TypeId metaType() const
    {
        return m_type;
    }

    /// Returns true if the class managed by this meta-class is abstract.
    bool isAbstract() const
    {
        return m_isAbstract;
    }

    /// Check whether the MetaClass is the class of the passed MetaObject.
    virtual bool isClassOf(const MetaObject&) const = 0;

    /// Casts the instance to a std::any using the class type reflected by the metaclass.
    /// \param instance The instance to convert.
    /// \return The std::any holding the value casted using the class type reflected.
    virtual std::any castInstance(void* instance) const = 0;

protected:
    /// Creates a metaclass with a registered MetaType identifier.
    explicit MetaClass(MetaType::TypeId type, bool abstract);

    void addMethod(MetaMethod* method);

    typedef std::vector<const MetaClass*> MetaClassContainer;
    typedef std::vector<const MetaMethod*> MetaMethodContainer;

    MetaClassContainer m_superClasses;
    MetaMethodContainer m_methods;
    MetaType::TypeId m_type;
    const bool m_isAbstract:1;

    friend class MetaMethod;

private:
    byte __padding[3];
};

/// MetaClass template specialized on non-MetaObject base classes, aswell as for interfaces.
template <class Class, class... SuperClasses>
struct InterfaceMetaClass : MetaClass
{
    static constexpr bool abstract = std::is_abstract_v<Class>;

    explicit InterfaceMetaClass()
        : MetaClass(MetaType::registerMetaType<Class>().id(), abstract)
    {
        static_assert(!std::is_base_of<MetaObject, Class>::value, "InterfaceMetaClassImpl reflects a non-MetaObject class.");
        std::array<const MetaClass*, sizeof... (SuperClasses)> aa =
        {{
            SuperClasses::getStaticMetaClass()...
        }};
        m_superClasses = MetaClassContainer(aa.begin(), aa.end());
    }
    /// Checks whether the \a metaObject is derived from this interface.
    bool isClassOf(const MetaObject& metaObject) const override
    {
        return dynamic_cast<const Class*>(&metaObject) != nullptr;
    }

    std::any castInstance(void* instance) const override
    {
        return reinterpret_cast<Class*>(instance);
    }
};

/// MetaClass template specialized on MetaObject-derived classes. The super-classes are interface
/// classes with reflection.
template <class Class, class... SuperClasses>
struct ObjectMetaClass : MetaClass
{
    static constexpr bool abstract = std::is_abstract_v<Class>;

    explicit ObjectMetaClass()
        : MetaClass(MetaType::registerMetaType<Class>().id(), abstract)
    {
        static_assert(std::is_base_of<MetaObject, Class>::value, "MetaClassImpl reflects a MetaObject derived class.");
        std::array<const MetaClass*, sizeof... (SuperClasses)> aa =
        {{
            SuperClasses::getStaticMetaClass()...
        }};
        m_superClasses = MetaClassContainer(aa.begin(), aa.end());
    }

    /// Checks whether this MetaClass reflects the \a metaObject.
    bool isClassOf(const MetaObject& metaObject) const override
    {
        return dynamic_cast<const Class*>(&metaObject) != nullptr;
    }
    std::any castInstance(void* instance) const override
    {
        return reinterpret_cast<Class*>(instance);
    }
};

/// Base class for the classes that provide standalone type reflection.
class MOX_API MetaObject
{
public:
    /// Destructor.
    virtual ~MetaObject();

    /// Returns the static metaclass of the metaobject.
    static const MetaClass* getStaticMetaClass();

    /// Returns the dynamic metaclass of the metaobject.
    virtual const MetaClass* getDynamicMetaClass() const;

protected:
    /// Constructor.
    explicit MetaObject();
};

} // namespace mox

/// Declares the MetaClass of a non-MetaObject derived base class or interface. The meta-class name associated to the
/// \a Class is built by appending the MetaClass to the class' name. E.g. A class BaseClass gets BaseClassMetaClass
/// as meta-class name. Use the macro when defining a class that has no base meta-classes.
#define MIXIN_METACLASS_BASE(Class) \
    static const mox::MetaClass* getStaticMetaClass() \
    { \
        static Class##MetaClass metaObject; \
        return &metaObject; \
    } \
    struct MOX_API Class##MetaClass : mox::InterfaceMetaClass<Class> \

/// Declares the MetaClass of a non-MetaObject derived base class or interface. The meta-class name associated to the
/// \a Class is built by appending the MetaClass to the class' name. E.g. A class BaseClass gets BaseClassMetaClass
/// as meta-class name. Use the macro when defining a class that has base meta-classes.
#define MIXIN_METACLASS(Class, ...) \
    static const mox::MetaClass* getStaticMetaClass() \
    { \
        static Class##MetaClass metaObject; \
        return &metaObject; \
    } \
    struct MOX_API Class##MetaClass : mox::InterfaceMetaClass<Class, __VA_ARGS__> \

/// Declares the MetaClass of a MetaObject derived base class or interface. The meta-class name associated to the
/// \a Class is built by appending the MetaClass to the class' name. E.g. A class BaseClass gets BaseClassMetaClass
/// as meta-class name.
#define METACLASS(Class, ...) \
    static const mox::MetaClass* getStaticMetaClass() \
    { \
        static Class##MetaClass metaObject; \
        return &metaObject; \
    } \
    const mox::MetaClass* getDynamicMetaClass() const override \
    { \
        return getStaticMetaClass(); \
    } \
    struct MOX_API Class##MetaClass : mox::ObjectMetaClass<Class, __VA_ARGS__>

#endif // METACLASS_HPP
