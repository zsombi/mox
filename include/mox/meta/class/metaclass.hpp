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
#include <mox/meta/core/variant.hpp>
#include <mox/meta/core/callable.hpp>
#include <mox/meta/core/metatype_descriptor.hpp>

#include <mox/meta/signal/signal.hpp>
#include <mox/meta/signal/signal_type.hpp>
#include <mox/meta/property/property_type.hpp>

/// Static metaclass declarator.
/// \param ThisClass
/// \param BaseClass
/// \param SuperClasses...
#define MetaInfo(...)                               \
    struct StaticMetaClass;                         \
    static StaticMetaClass* __getStaticMetaClass()  \
    {                                               \
        static StaticMetaClass metaClass;           \
        return &metaClass;                          \
    }                                               \
    struct MOX_API StaticMetaClass : mox::metainfo::StaticMetaClass<StaticMetaClass, __VA_ARGS__>

namespace mox
{

/// Template function, registers a class' metaclass to the metatype subsystem. The function
/// registers the static and the pointer type of the class to the metatype.
/// \tparam ClassType The class type to register to the metadata
/// \param name The metatype name of the class. If empty, the type is registered with the
/// default ABI name.
/// \return The pair of metatypes with the static and pointer variant of the class.
template <class ClassType>
std::pair<Metatype, Metatype> registerMetaClass(std::string_view name = "");

class MetaObject;
struct MetaClass;
class PropertyType;
class MethodType;

namespace metainfo
{

/// Base class for types with meta information.
class MOX_API AbstractMetaInfo
{
public:
    virtual ~AbstractMetaInfo() = default;

    /// Returns the name of the metainfo.
    /// \return The name of the metainfo.
    std::string name() const;

    /// Returns the signature of the metainfo.
    virtual std::string signature() const = 0;

protected:
    /// Constructor.
    explicit AbstractMetaInfo(std::string_view name);

private:
    std::string m_name;
};

/// MetaClass represents the type reflection or metadata of a managed structure or class. The metadata consists
/// of factory functions, methods, properties and signals.
///
struct MOX_API MetaClass
{
private:
    class MOX_API MetaSignalBase : public SignalType, public AbstractMetaInfo
    {
    public:
        explicit MetaSignalBase(MetaClass& hostClass, VariantDescriptorContainer&& args, std::string_view name);
        std::string signature() const override;
    };

    class MOX_API MetaPropertyBase : public PropertyType, public AbstractMetaInfo
    {
    public:
        explicit MetaPropertyBase(MetaClass& hostClass, VariantDescriptor&& typeDes, PropertyAccess access, const MetaSignalBase& signal, PropertyDataProviderInterface& defaultValue, std::string_view name);
        std::string signature() const override;
    };

    class MOX_API MetaMethodBase : public Callable, public AbstractMetaInfo
    {
    public:
        template <typename Function>
        MetaMethodBase(MetaClass& hostClass, Function method, std::string_view name)
            : Callable(method)
            , AbstractMetaInfo(name)
        {
            hostClass.addMetaMethod(*this);
        }
        std::string signature() const override;
    };

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
    using MethodVisitor = std::function<bool(const Callable*, const AbstractMetaInfo&)>;
    /// Signal visitor function.
    using SignalVisitor = std::function<bool(const SignalType*, const AbstractMetaInfo&)>;
    /// Property visitor function.
    using PropertyVisitor = std::function<bool(const PropertyType*, const AbstractMetaInfo&)>;
    /// Metaclass visitor function.
    using MetaClassVisitor = std::function<VisitorResultType(const MetaClass&)>;

    /// Metasignal class, declares a signal type with an associated name.
    template <class HostClass, typename... Arguments>
    class MOX_API MetaSignal : public MetaSignalBase
    {
    public:
        /// Constructor.
        MetaSignal(std::string_view name);

        /// Emits the signal on a sender object passing the arguments.
        template <typename... ConvertibleArgs>
        int emit(MetaBase& sender, ConvertibleArgs... arguments);
    };

    /// Metaproperty class, declares a property type with an associated name.
    template <class HostClass, typename ValueType, PropertyAccess access>
    class MOX_API MetaProperty : private PropertyDefaultValue<ValueType>, public MetaPropertyBase
    {
    public:
        /// Constructor.
        MetaProperty(const MetaSignalBase& sigChanged, std::string_view name, const ValueType& defaultValue = ValueType());
    };

    /// Metamethod class, declares a callable with an associated name.
    template <class HostClass>
    class MOX_API MetaMethod : public MetaMethodBase
    {
    public:
        /// Constructor.
        template <typename MethodType>
        MetaMethod(MethodType method, std::string_view name);
    };

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
    void addMetaMethod(Callable& method);
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
    const Callable* visitMethods(const MethodVisitor& visitor) const;

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
    using MetaMethodContainer = std::vector<const MetaMethodBase*>;
    using MetaSignalContainer = std::vector<const MetaSignalBase*>;
    using MetaPropertyContainer = std::vector<const MetaPropertyBase*>;

    /// Creates a metaclass with a registered MetatypeDescriptor identifier.
    explicit MetaClass(std::pair<Metatype, Metatype> type);

    MetaMethodContainer m_metaMethods;
    MetaSignalContainer m_metaSignals;
    MetaPropertyContainer m_metaProperties;
    std::pair<Metatype, Metatype> m_type;
};


/// Template for static metaclasses. Use this to define your metaclass for your classes.
/// Use MetaInfo() to declare the metaclass for your class.
template <class MetaClassDecl, class BaseClass, class... SuperClasses>
struct StaticMetaClass : MetaClass
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


/// Scans metaclasses and returns the metaclass for which the \a predicate returns \e true.
/// \param predicate The predicate to test a metaclass descriptor.
/// \return The pointer to the MetaClass found, nullptr if the predicate didn't find a match.
MOX_API const MetaClass* find(std::function<bool(const MetaClass&)> predicate);

/// \name Meta-invocators
/// \{
/// Invokes a signal on an \a instance identified by \a signalName, passing the given \a arguments.
/// The instance must have a metaclass defined. Returns the number of times the signal connections
/// were invoked, or -1 if there was no signal defined on the instance with the given name.
/// \param instance The instance of the class.
/// \param signalName The name of the metasignal to invoke.
/// \param args The arguments to pass. If the signal has no arguments, pass nothing.
/// \returns Returns emit count, or -1 if the signal is not defined on the instance.
template <class Class, typename... Arguments>
int emit(Class& instance, std::string_view signalName, Arguments... arguments);

/// Invokes a method on an \a instance, passing the given \a arguments. The instance must
/// have a metaclass defined.
/// \param instance The instance of the class.
/// \param methodName The name of the metamethod to invoke.
/// \param arguments The arguments to pass. If the invokable has no arguments, pass nothing.
/// \returns If the metamethod is found on the instance, returns \e true, otherwise \e false.
template <class Class, typename... Arguments>
std::optional<Variant> invoke(Class& instance, std::string_view methodName, Arguments... arguments);

/// Return a \a property value from \a instance.
/// \tparam ValueType The type of the property.
/// \tparam Class The class type for the instance that holds the property.
/// \param instance The instance where the property with \a name is hosted.
/// \param property The property name.
/// \return An optional with the property value. \e std::nullopt is returned if the property is not found
/// on the instance.
template <typename ValueType, class Class>
std::optional<ValueType> getProperty(Class& instance, std::string_view property);

/// Set the value of a \a property on \a instance with the \a value passed as argument.
/// \tparam ValueType The type of the property.
/// \tparam Class The class type for the instance that holds the property.
/// \param property The property to set.
/// \param value The value to set on the property.
/// \return If the property value is set with success, returns \e true, otherwise returns \e false.
template <typename ValueType, class Class>
bool setProperty(Class& instance, std::string_view property, ValueType value);

/// Creates a connection between a \a signal and a \a metaMethod of a \a receiver.
/// \param receiver The receiver hosting the metamethod.
/// \param metaMethod The metamethod to connect to.
/// \return The connection shared object.
MOX_API Signal::ConnectionSharedPtr connect(Signal& signal, MetaBase& receiver, const Callable& metaMethod);

/// Connects a \a signal from \a sender to a \a slot in \a receiver.
/// \tparam Sender The sender class type.
/// \tparam Receiver The receiver class type.
/// \param sender The sender object.
/// \param signal The signal name in sender.
/// \param receiver The receiver object.
/// \param slot The name of the slot, the metamethod to connect.
/// \return If the connection succeeds, returns the connection object. If the connection fails, returns \e nullptr.
template <class Sender, class Receiver>
Signal::ConnectionSharedPtr connect(Sender& sender, std::string_view signal, Receiver& receiver, std::string_view slot);
/// \}

} // metainfo

} // namespace mox

#include <mox/meta/class/detail/metaclass_impl.hpp>

#endif // METACLASS_HPP
