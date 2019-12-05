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

#include <vector>
#include <string_view>

#include <mox/config/deftypes.hpp>
#include <mox/metadata/variant.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <mox/utils/globals.hpp>

namespace mox
{

class MetaObject;
struct MetaClass;
class SignalType;
class PropertyType;
class MethodType;

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

    using VisitorResultType = std::tuple<VisitorResult, MetaValue>;
    /// Method visitor function.
    using MethodVisitor = std::function<bool(const MethodType*)>;
    /// Signal visitor function.
    using SignalVisitor = std::function<bool(const SignalType*)>;
    /// Property visitor function.
    using PropertyVisitor = std::function<bool(const PropertyType*)>;
    /// Metaclass visitor function.
    using MetaClassVisitor = std::function<VisitorResultType(const MetaClass&)>;

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

    /// Adds a \a method to the metaclass.
    void addMetaMethod(MethodType& method);
    /// Adds a \s signal to the metaclass.
    void addMetaSignal(SignalType& signal);
    /// Adds a \a property to the metaclass.
    void addMetaProperty(PropertyType& property);

    /// Tests whether this MetaClass is the superclass of the \a metaClass passed as argument.
    bool isSuperClassOf(const MetaClass& metaClass) const;

    /// Tests whether this MetaClass derives from the \a metaClass passed as argument.
    bool derivesFrom(const MetaClass& metaClass) const;

    /// Returns the MetaClass that manages the \a className class.
    static const MetaClass* find(std::string_view className);

    /// Visits the metaclass passing the methods to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta method for which the visitor returns \e true, \e nullptr if
    /// no method is identified by the visitor.
    const MethodType* visitMethods(const MethodVisitor& visitor) const;

    /// Visits the metaclass passing the signals to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta signal for which the visitor returns \e true, \e nullptr if
    /// no signal is identified by the visitor.
    const SignalType* visitSignals(const SignalVisitor& visitor) const;

    /// Visits the metaclass passing the properties to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta-property for which the visitor returns \e true, \e nullptr if
    /// no property is identified by the visitor.
    const PropertyType* visitProperties(const PropertyVisitor& visitor) const;

    /// Returns the pair of metatypes representing the static and dynamic types of the MetaClass.
    const std::pair<Metatype, Metatype>& getMetaTypes() const
    {
        return m_type;
    }

    /// Returns true if the class managed by this meta-class is abstract.
    virtual bool isAbstract() const = 0;

    /// Check whether the MetaClass is the class of the passed MetaObject.
    virtual bool isClassOf(const MetaObject&) const = 0;

protected:
    using MetaMethodContainer = std::vector<const MethodType*>;
    using MetaSignalContainer = std::vector<const SignalType*>;
    using MetaPropertyContainer = std::vector<const PropertyType*>;

    /// Creates a metaclass with a registered MetatypeDescriptor identifier.
    explicit MetaClass(std::pair<Metatype, Metatype> type);

    MetaMethodContainer m_metaMethods;
    MetaSignalContainer m_metaSignals;
    MetaPropertyContainer m_metaProperties;
    std::pair<Metatype, Metatype> m_type;
};


/// Template for static metaclasses. Use this to define your metaclass for your classes.
/// Use ClassMetaData() to declare the metaclass for your class.
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
struct StaticMetaClass : mox::MetaClass
{
protected:
    VisitorResultType visitSuperClasses(const MetaClassVisitor& visitor) const override;

public:
    using DeclaredMetaClass = MetaClassDecl;
    using BaseType = BaseClass;

    StaticMetaClass();

    /// Override of MetaClass::isAbstract().
    bool isAbstract() const override;

    /// Checks whether the \a metaObject is derived from this interface.
    bool isClassOf(const MetaObject& metaObject) const override;

    /// Returns the static metaclass instance.
    static const DeclaredMetaClass* get();
};

/// Static metaclass declarator.
/// \param ThisClass
/// \param BaseClass
/// \param SuperClasses...
#define ClassMetaData(...)                                                              \
    struct StaticMetaClass;                                                             \
    static StaticMetaClass* __getStaticMetaClass()                                      \
    {                                                                                   \
        static StaticMetaClass metaClass;                                               \
        return &metaClass;                                                              \
    }                                                                                   \
    struct MOX_API StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, __VA_ARGS__>

/// Template function, registers a class' metaclass to the metatype subsystem. The function
/// registers the static and the pointer type of the class to the metatype.
/// \tparam ClassType The class type to register to the metadata
/// \param name The metatype name of the class. If empty, the type is registered with the
/// default ABI name.
/// \return The pair of metatypes with the static and pointer variant of the class.
template <class ClassType>
std::pair<Metatype, Metatype> registerMetaClass(std::string_view name = "");

} // namespace mox

#include <mox/metadata/detail/metaclass_impl.hpp>

#endif // METACLASS_HPP
