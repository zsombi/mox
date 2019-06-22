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
#include <vector>

#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype_descriptor.hpp>

namespace mox
{

class MetaObject;
class MetaMethod;
class MetaSignal;

/// MetaClass represents the type reflection or metadata of a managed structure or class. The metadata consists
/// of factory functions, methods, properties and signals.
///
struct MOX_API MetaClass
{
public:
    /// Visitor result values.
    enum VisitorResult
    {
        /// Informs that visiting continues.
        Continue,
        /// Informs visiting abort.
        Abort
    };

    typedef std::tuple<VisitorResult, std::any> VisitorResultType;
    /// Method visitor function.
    typedef std::function<bool(const MetaMethod*)> MethodVisitor;
    /// Metasignal visitor function.
    typedef std::function<bool(const MetaSignal*)> SignalVisitor;
    /// Metaclass visitor function.
    typedef std::function<VisitorResultType(const MetaClass&)> MetaClassVisitor;

    /// Visits a metaclass and its superclasses. The superclasses are visited if the \a visitor
    /// tells to continue visiting..
    /// \param visitor The visitor function.
    /// \return The visiting result.
    VisitorResultType visit(const MetaClassVisitor& visitor) const;
    /// Visit the superclasses of a metaclass.
    /// \param visitor The visitor function.
    /// \return The visiting result.
    virtual VisitorResultType visitSuperClasses(const MetaClassVisitor& visitor) const;

    /// Destructor.
    virtual ~MetaClass();

    /// Tests whether this MetaClass is the superclass of the \a metaClass passed as argument.
    bool isSuperClassOf(const MetaClass& metaClass) const;

    /// Tests whether this MetaClass derives from the \a metaClass passed as argument.
    bool derivesFrom(const MetaClass& metaClass) const;

    /// Returns the MetaClass that manages the \a className class.
    static const MetaClass* find(std::string_view className);

    /// Visits the metaclass passing the methods to the \a visitor.
    /// \param visitor The visitor.
    /// \return The MetaMethod instance for which the visitor returns \e true, \e nullptr if
    /// no method is identified by the visitor.
    const MetaMethod* visitMethods(const MethodVisitor& visitor) const;

    /// Visits the metaclass passing the metasignals to the \a visitor.
    /// \param visitor The visitor.
    /// \return The MetaSignal instance for which the visitor returns \e true, \e nullptr if
    /// no metasignal is identified by the visitor.
    const MetaSignal* visitSignals(const SignalVisitor& visitor) const;

    /// Returns the Metatype of the MetaClass.
    Metatype metaType() const
    {
        return m_type.id();
    }

    /// Returns true if the class managed by this meta-class is abstract.
    virtual bool isAbstract() const = 0;

    /// Check whether the MetaClass is the class of the passed MetaObject.
    virtual bool isClassOf(const MetaObject&) const = 0;

    /// Casts the instance to a std::any using the class type reflected by the metaclass.
    /// \param instance The instance to convert.
    /// \return The std::any holding the value casted using the class type reflected.
    virtual std::any castInstance(void* instance) const = 0;

protected:
    typedef std::vector<const MetaMethod*> MetaMethodContainer;
    typedef std::vector<const MetaSignal*> MetaSignalContainer;

    /// Creates a metaclass with a registered MetatypeDescriptor identifier.
    explicit MetaClass(const MetatypeDescriptor& type);

    void addMethod(MetaMethod* method);
    size_t addSignal(MetaSignal& signal);

    MetaMethodContainer m_methods;
    MetaSignalContainer m_signals;
    const MetatypeDescriptor& m_type;

    friend class MetaMethod;
    friend class MetaSignal;
};

namespace decl
{

template <class MetaClassDecl, class BaseClass, class... SuperClasses>
struct MetaClass : mox::MetaClass
{
protected:
    VisitorResultType visitSuperClasses(const MetaClassVisitor& visitor) const override
    {
        std::array<const mox::MetaClass*, sizeof... (SuperClasses)> supers = {{SuperClasses::StaticMetaClass::get()...}};
        for (const mox::MetaClass* metaClass : supers)
        {
            VisitorResultType result = metaClass->visit(visitor);
            if (std::get<0>(result) == Abort)
            {
                return result;
            }
        }

        return std::make_tuple(Continue, std::any());
    }

public:
    static const mox::MetaClass* get()
    {
        static MetaClassDecl metaClass;
        return &metaClass;
    }

    explicit MetaClass()
        : mox::MetaClass(metatypeDescriptor<BaseClass>())
    {
    }

    bool isAbstract() const override
    {
        return std::is_abstract_v<BaseClass>;
    }

    /// Checks whether the \a metaObject is derived from this interface.
    bool isClassOf(const MetaObject& metaObject) const override
    {
        return dynamic_cast<const BaseClass*>(&metaObject) != nullptr;
    }

    std::any castInstance(void* instance) const override
    {
        return reinterpret_cast<BaseClass*>(instance);
    }
};

} // decl

} // namespace mox

/// Declares the static metaclass for a class that has no base metaclasses.
#define STATIC_METACLASS_BASE(Base) \
    struct MOX_API StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, Base>

/// Declare the static metaclass for a class that has base classes with metaclasses.
#define STATIC_METACLASS(Base, ...) \
    struct MOX_API StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, Base, __VA_ARGS__>

/// Declares the static metaclass for a class or interface, adding the dynamic metaclass fetching
/// function override. One of the base classes must declare the getMetaClass() method.
#define METACLASS(Class, ...) \
    const mox::MetaClass* getMetaClass() const override \
    { \
        return StaticMetaClass::get(); \
    } \
    struct MOX_API StaticMetaClass : mox::decl::MetaClass<StaticMetaClass, Class, __VA_ARGS__>

#endif // METACLASS_HPP
