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

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <list>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>
#include <mox/utils/globals.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metamethod.hpp>

#include <mox/utils/function_traits.hpp>

namespace mox
{

class SignalBase;
class SignalHost;

/// SignalBase is the base class of the Mox signals. You can declare signals using the Signal template class.
class MOX_API SignalBase
{
public:
    /// The class reptresents a connection to a signal. The connection is a token which holds the
    /// signal connected, and the function, method, metamethod, functor or lambda the signal is
    /// connected to. This function is called slot.
    class MOX_API Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        /// Destructor.
        virtual ~Connection() = default;

        /// Returns the state of the connection.
        /// \return If teh connection is connected, \e true, otherwise \e false.
        virtual bool isConnected() const = 0;

        /// Disconnects the signal.
        /// \return If the disconnect succeeds, \e true. If the disconnect fails, \e false.
        bool disconnect();

        /// Tests a connection against the \a receiver and \a funcAddress passed as argument.
        /// \param receiver The receiver instance to check against.
        /// \param funcAddress The function address to test the connection against.
        /// \return If the connection holds the receiver and the function address passed as argument,
        /// returns \e true. Otherwise \e false.
        virtual bool compare(std::any receiver, const void* funcAddress) const;

    protected:
        /// Constructs a connection attached to the \a signal.
        explicit Connection(SignalBase& signal);

        /// Activates the connection by calling the slot of the connection.
        /// \param args The arguments to pass to the slot.
        virtual void activate(Callable::Arguments& args) = 0;

        /// Resets the connection.
        virtual void reset() = 0;

        /// The signal the connection is attached to.
        SignalBase& m_signal;

        friend class SignalBase;
    };

    /// The connection type.
    typedef std::shared_ptr<Connection> ConnectionSharedPtr;

    /// Returns the signal host instance.
    /// \return The signal host instance.
    SignalHost& host() const;

    /// Returns the signal identifier within a signal host.
    /// \return The signal identifier.
    size_t id() const;

    /// Checks the validity of a signal. A signal is invalid if it is no longer registered to
    /// a signal host.
    /// \return If the signal is valid, \e true, otherwise \e false.
    bool isValid() const;

    /// Creates a connection between a signal and a \a metaMethod of a \a receiver.
    /// \param receiver The receiver hosting the metamethod.
    /// \param metaMethod The metamethod to connect to.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(std::any receiver, const MetaMethod& metaMethod);

    /// Creates a connection between this signal and a receiver \a signal.
    /// \param signal The receiver signal connected to this signal.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(const SignalBase& signal);

    /// Activates the connections of the signal by invoking the slots from each connection passing
    /// the \a arguments to the slots. Connections created during the activation are not invoked
    /// in the same activation cicle.
    /// \param arguments The arguments to pass to the slots, being the arguments passed to the signal.
    /// \return The number of connections activated.
    size_t activate(Callable::Arguments& arguments);

protected:
    SignalBase() = delete;
    SignalBase(const SignalBase&) = delete;
    SignalBase& operator=(const SignalBase&) = delete;

    /// Constructs a signal registering it to the signal \a host with the \a name passed as argument.
    explicit SignalBase(SignalHost& host, std::string_view name);
    /// Destructor.
    virtual ~SignalBase();

    /// Adds a \a connection to the signal.
    void addConnection(ConnectionSharedPtr connection);
    /// Removes a \a connection from the signal.
    void removeConnection(ConnectionSharedPtr connection);

    /// Creates a connection to a \a lambda. The connection owns the callable.
    ConnectionSharedPtr connect(Callable&& lambda);
    /// Creates a connection to a \a receiver and a \a slot. The connection owns the callable.
    ConnectionSharedPtr connect(std::any receiver, Callable&& slot);

    /// Disconnects a connection that holds a \a receiver and \a callableAddress.
    bool disconnect(std::any receiver, const void* callableAddress);
    /// Disconnects a connection that holds the \a signal.
    bool disconnect(const SignalBase& signal);

    /// Connection container type.
    typedef std::vector<ConnectionSharedPtr> ConnectionList;

    /// The signal host address.
    SignalHost& m_host;
    /// The name of the signal.
    std::string m_name;
    /// The collection of active connections.
    ConnectionList m_connections;
    /// The signal identifier
    size_t m_id;
    /// Triggering flag. Locks the signal from recursive triggering.
    bool m_triggering;

    friend class SignalHost;
};

/// The class is the counterpart of the Mox signals, it holds all the signals declared on a class.
/// Each class that declares signals must be derived from SignalHost class.
class MOX_API SignalHost
{
public:
    /// Destructor.
    virtual ~SignalHost();

protected:
    /// Constructor.
    explicit SignalHost() = default;

    /// Signal host lock, guards the signal register.
    std::mutex m_lock;
    /// The signal register holding all declared signals on a signal host.
    std::vector<const SignalBase*> m_signals;

    /// Registers the \a signal to the signal host.
    /// \param signal The signal to register.
    /// \return The signal identifier.
    size_t registerSignal(SignalBase& signal);

    /// Removes a signal from the signal host register.
    /// \param signal The signal to remove from the register.
    void removeSignal(SignalBase& signal);

    friend class SignalBase;
};

/// Signal template specialization for a generic signature.
template <typename SignatureFunction>
class Signal;

/// Signal template, specialization with a signature of void function with arbitrary arguments.
/// The arguments must be registered as metatypes. You can connect methods, metamethods, functions,
/// functors or lambdas to a signal using the relevant connect() methods. You can disconnect the
/// connections either calling disconnect() on the signal, or by calling SignalBase::Connection::disconnect().
/// \note You can connect signals to slots that have the same arguments or less arguments than
/// the signal has. When connecting to slots with less arguments than the signal signature, only
/// slots that have the same argument types at the specific argument index are considered connectable.
/// This is also valid when connecting two signals.
template <typename... Args>
class Signal<void(Args...)> : public SignalBase
{
    static constexpr size_t arity = sizeof... (Args);
    const std::array<ArgumentDescriptor, arity> m_argumentDescriptors = {{ ArgumentDescriptor::get<Args>()... }};

    template <typename Signature> friend class MetaSignal;

public:
    /// The signature of the signal.
    typedef void(*Signature)(Args...);

    /// Returns an array with the argument descriptors of the signal.
    Callable::ArgumentDescriptorContainer argumentDescriptors() const;

    /// Constructs the signal attaching it to the \a owner.
    /// \param owner The SignalHost owning the signal.
    /// \param name The name of the signal.
    explicit Signal(SignalHost& owner, std::string_view name)
        : SignalBase(owner, name)
    {
    }

    /// Signal emitter. Packs the \a arguments into a Callable::Arguments pack and activates
    /// the signal connections.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections activated.
    size_t operator()(Args... arguments);

    /// Connects a metamethod with \a methodName. The metamethod must be registered in the \a receiver's
    /// static or dynamic metaclass.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <class Receiver>
    ConnectionSharedPtr connect(const Receiver& receiver, const char* methodName);

    /// Connects a \a method of a \a receiver thos this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename SlotFunction>
    typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, ConnectionSharedPtr>::type
    connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Connects a \a signal to this signal.
    /// \param method The method to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename ReceiverSignal>
    typename std::enable_if<std::is_base_of_v<SignalBase, ReceiverSignal>, ConnectionSharedPtr>::type
    connect(const ReceiverSignal& signal);

    /// Connects a \a function, or a lambda to this signal.
    /// \param function The function, functor or lambda to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename Function>
    typename std::enable_if<!std::is_base_of_v<SignalBase, Function>, ConnectionSharedPtr>::type
    connect(const Function& function);

    /// Disconnects a metamethod with \a methodName. The metamethod must be registered in the \a receiver's
    /// static or dynamic metaclass.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the method with the \a methodName and \a receiver was connected, and the disconnect succeeded,
    /// returns \e true. Otherwise returns \e false.
    template <typename Receiver>
    bool disconnect(const Receiver& receiver, const char* methodName);

    /// Disconnects a \a method that is a method of the \a receiver.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the method with the \a methodName and \a receiver was connected, and the disconnect
    /// succeeded, returns \e true. Otherwise returns \e false.
    template <typename SlotFunction>
    typename std::enable_if<std::is_member_function_pointer_v<SlotFunction>, bool>::type
    disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Disonnects a \a function, functor or a lambda from this signal.
    /// \param function The function, functor or lambda to disconnect.
    /// \return If the \a function, functor or lambda was connected, and the disconnect succeeded,
    /// returns \e true. Otherwise returns \e false.
    template <typename SlotFunction>
    typename std::enable_if<!std::is_base_of_v<SignalBase, SlotFunction>, bool>::type
    disconnect(const SlotFunction& slot);

    /// Disconnects a \a signal from this signal.
    /// \param signal The signal to disconnect.
    /// \return If the \a signal was connected, and the disconnect succeeded, returns \e true.
    /// Otherwise returns \e false.
    template <typename SignalType>
    typename std::enable_if<std::is_base_of_v<SignalBase, SignalType>, bool>::type
    disconnect(const SignalType& signal);
};

class MOX_API MetaSignal
{
protected:
    explicit MetaSignal(MetaClass& metaClass, std::string_view name, const Callable::ArgumentDescriptorContainer& args);
    virtual ~MetaSignal() =  default;

    MetaClass& m_ownerClass;
    Callable::ArgumentDescriptorContainer m_arguments;
    std::string m_name;
};

template <typename Signature>
class MOX_API MetaSignalImpl : public MetaSignal
{
public:
    explicit MetaSignalImpl(MetaClass& metaClass, std::string_view name)
        : MetaSignal(metaClass, name, function_traits<Signature>::argument_descriptors())
    {
    }
};

} // mox

#include <mox/metadata/detail/signal_impl.hpp>

#endif // SIGNAL_HPP
