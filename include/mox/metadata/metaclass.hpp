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
struct SignalDescriptorBase;

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
        /// Constructs a metasignal from a \a descriptor witha  given \a name, and attaches it to the \a metaClass.
        explicit Signal(MetaClass& metaClass, const SignalDescriptorBase& descriptor, std::string_view name);

        /// Returns the name of the metasignal.
        /// \return The name of the metasignal.
        std::string name() const;

        /// Returns the descriptor of the metasignal.
        const SignalDescriptorBase& descriptor() const;

        /// Tests whether this signal is invocable with the given \a arguments.
        bool isInvocableWith(const VariantDescriptorContainer& arguments) const;

    private:
        MetaClass& m_ownerClass;
        const SignalDescriptorBase& m_descriptor;
        std::string m_name;

        Signal(Signal const&) = delete;
        Signal(Signal&&) = delete;
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

    /// Invokes a metasignal on an \a instance with the given \a arguments
    template <typename Class, typename... Arguments>
    int emit(Class& instance, const Signal& signal, Arguments... arguments) const;

    /// Visitor result values.
    enum VisitorResult
    {
        /// Informs that visiting continues.
        Continue,
        /// Informs visiting abort.
        Abort
    };

    typedef std::tuple<VisitorResult, MetaValue> VisitorResultType;
    /// Method visitor function.
    typedef std::function<bool(const Method*)> MethodVisitor;
    /// Signal visitor function.
    typedef std::function<bool(const Signal*)> SignalVisitor;
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
    /// \return The meta method for which the visitor returns \e true, \e nullptr if
    /// no method is identified by the visitor.
    const Method* visitMethods(const MethodVisitor& visitor) const;

    /// Visits the metaclass passing the signals to the \a visitor.
    /// \param visitor The visitor.
    /// \return The meta signal for which the visitor returns \e true, \e nullptr if
    /// no method is identified by the visitor.
    const Signal* visitSignals(const SignalVisitor& visitor) const;

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

    void addMethod(const Method& method);
    void addSignal(const Signal& signal);

    /// Creates a metaclass with a registered MetatypeDescriptor identifier.
    explicit MetaClass(const MetatypeDescriptor& type);

    MethodContainer m_methods;
    SignalContainer m_signals;
    const MetatypeDescriptor& m_type;
};


/// Template for static metaclasses. Use this to define your metaclass for your classes.
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
struct StaticMetaClass : mox::MetaClass
{
protected:
    VisitorResultType visitSuperClasses(const MetaClassVisitor& visitor) const override;

public:
    using BaseType = BaseClass;

    static const MetaClassDecl* get()
    {
        static MetaClassDecl metaClass;
        return &metaClass;
    }

    explicit StaticMetaClass();

    /// Override of MetaClass::isAbstract().
    bool isAbstract() const override;

    /// Checks whether the \a metaObject is derived from this interface.
    bool isClassOf(const MetaObject& metaObject) const override;
};

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

} // namespace mox

#include <mox/metadata/detail/metaclass_impl.hpp>

#endif // METACLASS_HPP
