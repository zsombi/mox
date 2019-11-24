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
#include <optional>

#include <mox/metadata/variant.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metatype_descriptor.hpp>
#include <mox/utils/globals.hpp>

namespace mox
{

class MetaObject;
class SignalType;
class PropertyType;

/// MetaClass represents the type reflection or metadata of a managed structure or class. The metadata consists
/// of factory functions, methods, properties and signals.
///
struct MOX_API MetaClass
{
public:
    /// Meta method is a callable that is attached to a MetaClass, and typically holds a method of a class.
    /// However, you can add static methods, functions or lambdas to the metaclass of your class, declared
    /// outside of the class' scope. These functions do not get passed the class instance they are invoked
    /// on, however invoking these types of metamethods require the class instance.
    ///
    /// MetaMethods are invoked using metaInvoke() template function, passing the instance, the name of
    /// the method and the eventual arguments that are forwarded to the method.
    class MOX_API Method : public Callable
    {
    public:
        /// Constructs a metamethod with the \a function identified with \a name, and attaches to the \a metaClass.
        template <typename Function>
        explicit Method(MetaClass& metaClass, Function fn, std::string_view name);

        /// Returns the name of the metamethod.
        /// \return The name of the metamethod.
        std::string name() const;

    private:
        MetaClass& m_ownerClass;
        std::string m_name;

        DISABLE_COPY(Method)
        DISABLE_MOVE(Method)
    };

    /// Meta signals are links to the signal descriptors of a class. They expose metainformation to the
    /// metadata.
    class MOX_API Signal
    {
    public:
        /// Constructs a metasignal from a \a type with a given \a name, and attaches it to the \a metaClass.
        explicit Signal(MetaClass& metaClass, const SignalType& type, std::string_view name);

        /// Returns the name of the metasignal.
        /// \return The name of the metasignal.
        std::string name() const;

        /// Returns the descriptor of the metasignal.
        const SignalType& type() const;

        /// Activates a metasignal with a \a sender and \a arguments.
        /// \return The activation count, -1 on error.
        int activate(intptr_t sender, const Callable::ArgumentPack& arguments) const;

        /// Emit a metasignal with the SenderObject and Arguments
        /// \param sender
        /// \param args
        /// \return The activation count, -1 on error.
        template <class SenderObject, typename... Arguments>
        int emit(SenderObject& sender, Arguments... args) const;

        /// Tests whether this signal is invocable with the given \a arguments.
        bool isInvocableWith(const VariantDescriptorContainer& arguments) const;

    private:
        MetaClass& m_ownerClass;
        const SignalType& m_type;
        std::string m_name;

        DISABLE_COPY(Signal)
        DISABLE_MOVE(Signal)
    };

    /// Meta-properties provide the metatype for the properties declared in a metadata enabled class.
    class MOX_API Property
    {
    public:
        /// Constructor.
        explicit Property(MetaClass& metaClass, PropertyType& type, std::string_view name);

        /// Returns the name of the metaproperty.
        /// \return The name of the metaproperty.
        std::string name() const;

        /// Returns the metaproeprty type.
        const PropertyType& type() const;

        /// Metaproperty getter, returns the value of a property associated to the metaproperty
        /// and an instance.
        /// \param instance The instance that holds this property.
        /// \return The property value in variant. If the instance does not contain the property,
        /// returns an invalid variant.
        Variant get(intptr_t instance) const;

        /// Metaproperty setter, sets the value of a proeprty on an instance with a given value.
        /// \param instance The instance that holds this property.
        /// \param value The value to set.
        /// \return If the property is defined on the instance, and the value is set with success,
        /// returns \e true, otherwise \e false.
        bool set(intptr_t instance, const Variant& value) const;

    private:
        DISABLE_COPY(Property)
        DISABLE_MOVE(Property)

        MetaClass& m_ownerClass;
        PropertyType& m_type;
        std::string m_name;
    };

    /// Invokes a metamethod on an \a instance with the given \a arguments.
    /// \param instance The instance that possibly owns the method.
    /// \param method The metamethod to invoke.
    /// \param arguments The variadic arguments to pass as argument to the metamethod.
    /// \returns If the metamethod is invokable with the passed arguments, returns the return value as
    /// a variant, or an invalid variant object. If the metamethod is not invokable with the arguments,
    /// or the method does not belong to the metaclass of the instance, returns \e nullopt.
    template <class Class, typename... Arguments>
    std::optional<Variant> invoke(Class& instance, const Method& method, Arguments... arguments) const;

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
    using MethodVisitor = std::function<bool(const Method*)>;
    /// Signal visitor function.
    using SignalVisitor = std::function<bool(const Signal*)>;
    /// Property visitor function.
    using PropertyVisitor = std::function<bool(const Property*)>;
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
    const Method* visitMethods(const MethodVisitor& visitor) const;

    /// Visits the metaclass passing the signals to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta signal for which the visitor returns \e true, \e nullptr if
    /// no signal is identified by the visitor.
    const Signal* visitSignals(const SignalVisitor& visitor) const;

    /// Visits the metaclass passing the properties to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta-property for which the visitor returns \e true, \e nullptr if
    /// no property is identified by the visitor.
    const Property* visitProperties(const PropertyVisitor& visitor) const;

    /// Returns the Metatype of the MetaClass.
    Metatype metaType() const
    {
        return m_type.id();
    }

    /// Returns true if the class managed by this meta-class is abstract.
    virtual bool isAbstract() const = 0;

    /// Check whether the MetaClass is the class of the passed MetaObject.
    virtual bool isClassOf(const MetaObject&) const = 0;

protected:
    typedef std::vector<const Method*> MethodContainer;
    typedef std::vector<const Signal*> SignalContainer;
    using PropertyContainer = std::vector<const Property*>;

    void addMethod(const Method& method);
    void addSignal(const Signal& signal);
    void addProperty(const Property& property);

    /// Creates a metaclass with a registered MetatypeDescriptor identifier.
    explicit MetaClass(const MetatypeDescriptor& type);

    MethodContainer m_methods;
    SignalContainer m_signals;
    PropertyContainer m_properties;
    const MetatypeDescriptor& m_type;
};


/// Template for static metaclasses. Use this to define your metaclass for your classes.
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
struct StaticMetaClass : mox::MetaClass
{
protected:
    VisitorResultType visitSuperClasses(const MetaClassVisitor& visitor) const override;

public:
    using DeclaredMetaClass = MetaClassDecl;
    using BaseType = BaseClass;

    explicit StaticMetaClass();

    /// Override of MetaClass::isAbstract().
    bool isAbstract() const override;

    /// Checks whether the \a metaObject is derived from this interface.
    bool isClassOf(const MetaObject& metaObject) const override;
};

#define MetaClassDefs()                             \
    static const DeclaredMetaClass* get()           \
    {                                               \
        static DeclaredMetaClass _staticMetaClass;  \
        return &_staticMetaClass;                   \
    }

template <class ClassType>
Metatype registerMetaClass(std::string_view name = "");

/// Invokes a method on an \a instance, passing the given \a arguments. The instance must
/// have a metaclass defined.
/// \param instance The instance of the class.
/// \param methodName The name of the metamethod to invoke.
/// \param arguments The arguments to pass. If the invokable has no arguments, pass nothing.
/// \returns If the metamethod is found on the instance, returns \e true, otherwise \e false.
template <class Class, typename... Arguments>
std::optional<Variant> invoke(Class& instance, std::string_view methodName, Arguments... arguments);

/// Invokes a signal on an \a instance identified by \a signalName, passing the given \a arguments.
/// The instance must have a metaclass defined. Returns the number of times the signal connections
/// were invoked, or -1 if there was no signal defined on the instance with the given name.
/// \param instance The instance of the class.
/// \param signalName The name of the metasignal to invoke.
/// \param args The arguments to pass. If the signal has no arguments, pass nothing.
/// \returns Returns emit count, or -1 if the signal is not defined on the instance.
template <class Class, typename... Arguments>
int emit(Class& instance, std::string_view signalName, Arguments... arguments);

template <typename ValueType, class Class>
std::pair<ValueType, bool> property(Class& instance, std::string_view name)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    auto finder = [&name](const MetaClass::Property* property)
    {
        return property->name() == name;
    };
    auto property = metaClass->visitProperties(finder);
    if (property)
    {
        ValueType value = property->get(intptr_t(&instance));
        return std::make_pair(value, true);
    }
    return std::make_pair(ValueType(), false);
}

template <typename ValueType, class Class>
bool setProperty(Class& instance, std::string_view name, ValueType value)
{
    const MetaClass* metaClass = Class::StaticMetaClass::get();
    auto finder = [&name](const MetaClass::Property* property)
    {
        return property->name() == name;
    };
    auto property = metaClass->visitProperties(finder);
    if (property)
    {
        return property->set(intptr_t(&instance), Variant(value));
    }
    return false;
}

} // namespace mox

#include <mox/metadata/detail/metaclass_impl.hpp>

#endif // METACLASS_HPP
