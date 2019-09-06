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

/// Metasignal declarator macro
#define META_SIGNAL(name)           mox::meta::MetaSignal<decltype(name)> meta_##name{*this, #name}

/// Declares a signal with \a name and \a signature. The signal must have a metasignal
/// declared with the same name.
#define SIGNAL(name, signature)     decl::Signal<signature> name{*this, #name}

namespace mox
{

class Signal;
class SignalHost;

/// MetaSignal holds the metadata of a signal.
class MOX_API MetaSignal
{
public:
    /// Returns the name of the signal.
    std::string name() const
    {
        return m_name;
    }

    /// Returns the ID of the signal.
    size_t id() const
    {
        return m_id;
    }

    /// Returns an array with the argument descriptors of the signal.
    const ArgumentDescriptorContainer& descriptors() const
    {
        return m_arguments;
    }

    /// Tests whether the signal is activable with the \a args.
    /// \param args The argument descriptors to test against.
    /// \return If the signal is activable with the arguments passed, returns \e true.
    /// Otherwise returns \e false.
    bool activableWith(const ArgumentDescriptorContainer& args) const;

protected:
    /// Creates a metasignal.
    explicit MetaSignal(MetaClass& metaClass, std::string_view name, const ArgumentDescriptorContainer& args);
    virtual ~MetaSignal() =  default;

    MetaClass& m_ownerClass;
    ArgumentDescriptorContainer m_arguments;
    std::string m_name;
    size_t m_id;
};

namespace meta
{

/// MetaSignal declarator.
template <class SignalType>
class MetaSignal : public mox::MetaSignal
{
public:
    explicit MetaSignal(MetaClass& metaClass, std::string_view name)
        : mox::MetaSignal(metaClass, name, function_traits<typename SignalType::Signature>::argument_descriptors())
    {
    }
};

} // namespace meta

/// Signal is the base class of the Mox signals. You can declare signals using the decl::Signal template class.
/// You can connect a signal to a method, a metamethod, a function, a functor or a lambda using one of the connect()
/// functions. The functions connected to a signal are called slots. These slots must have at maximum the same
/// amount and type of arguments as the signal they are connected to has. A slot that has different argument type
/// at a given argument index, or has more arguments than the signal signature has will fail to connect.
///
/// Optionally, a slot can have the first argument a Signal::ConnectionSharedPtr argument. When that is the case,
/// during the signal activation the connection that connects the slot with the signal is passed to the slot.
/// The slot can use this connection object to disconnect the slot from the signal.
class MOX_API Signal
{
public:
    /// The class represents a connection to a signal. The connection is a token which holds the
    /// signal connected, and the function, method, metamethod, functor or lambda the signal is
    /// connected to. This function is called slot.
    class MOX_API Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        /// Destructor.
        virtual ~Connection() = default;

        /// Returns the state of the connection.
        /// \return If the connection is connected, \e true, otherwise \e false.
        virtual bool isConnected() const = 0;

        Signal& signal() const;

        /// Disconnects the signal.
        /// \return If the disconnect succeeds, \e true. If the disconnect fails, \e false.
        bool disconnect();

        /// Tests a connection against the \a receiver and \a funcAddress passed as argument.
        /// \param receiver The receiver instance to check against.
        /// \param funcAddress The function address to test the connection against.
        /// \return If the connection holds the receiver and the function address passed as argument,
        /// returns \e true. Otherwise \e false.
        virtual bool compare(Argument receiver, const void* funcAddress) const;

    protected:
        /// Constructs a connection attached to the \a signal.
        explicit Connection(Signal& signal);

        /// Activates the connection by calling the slot of the connection.
        /// \param args The arguments to pass to the slot.
        virtual void activate(Callable::Arguments& args) = 0;

        /// Resets the connection.
        virtual void reset() = 0;

        /// The signal the connection is attached to.
        Signal& m_signal;
        /// Pass the connection object to the slot.
        bool m_passConnectionObject;

        friend class Signal;
    };

    /// The connection type.
    typedef std::shared_ptr<Connection> ConnectionSharedPtr;

    /// Returns the metasignal assiciated to the signal.
    /// \return The metasignal of the signal.
    const MetaSignal& metaSignal() const;

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

    /// Activates the connections of the signal by invoking the slots from each connection passing
    /// the \a arguments to the slots. Connections created during the activation are not invoked
    /// in the same activation cicle.
    /// \param arguments The arguments to pass to the slots, being the arguments passed to the signal.
    /// \return The number of connections activated.
    int activate(Callable::Arguments& arguments);


    /// Signal emitter. Packs the \a arguments into a Callable::Arguments pack and activates
    /// the signal connections.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections activated.
    template <typename... Args>
    int operator()(Args... arguments);

    /// Creates a connection between a signal and a \a metaMethod of a \a receiver.
    /// \param receiver The receiver hosting the metamethod.
    /// \param metaMethod The metamethod to connect to.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(Argument receiver, const MetaMethod& metaMethod);

    /// Creates a connection between this signal and a receiver \a signal.
    /// \param signal The receiver signal connected to this signal.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(const Signal& signal);

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
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionSharedPtr>
    connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param function The function, functor or lambda to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename Function>
    std::enable_if_t<!std::is_base_of_v<mox::Signal, Function>, ConnectionSharedPtr>
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
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, bool>
    disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Disonnects a \a function, functor or a lambda from this signal.
    /// \param function The function, functor or lambda to disconnect.
    /// \return If the \a function, functor or lambda was connected, and the disconnect succeeded,
    /// returns \e true. Otherwise returns \e false.
    template <typename SlotFunction>
    std::enable_if_t<!std::is_base_of_v<Signal, SlotFunction>, bool>
    disconnect(const SlotFunction& slot);

    /// Disconnects a \a signal from this signal.
    /// \param signal The signal to disconnect.
    /// \return If the \a signal was connected, and the disconnect succeeded, returns \e true.
    /// Otherwise returns \e false.
    bool disconnect(const Signal& signal);

protected:
    Signal() = delete;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// Constructs a signal with \a metaSignal, and registers it to the \a host passed as argument.
    explicit Signal(SignalHost& host, const MetaSignal& metaSignal);
    /// Destructor.
    virtual ~Signal();

    /// Adds a \a connection to the signal.
    void addConnection(ConnectionSharedPtr connection);
    /// Removes a \a connection from the signal.
    void removeConnection(ConnectionSharedPtr connection);

    /// Creates a connection to a \a lambda. The connection owns the callable.
    ConnectionSharedPtr connect(Callable&& lambda);
    /// Creates a connection to a \a receiver and a \a slot. The connection owns the callable.
    ConnectionSharedPtr connect(Argument receiver, Callable&& slot);

    /// Disconnects a connection that holds a \a receiver and \a callableAddress.
    bool disconnectImpl(Argument receiver, const void* callableAddress);

    /// Connection container type.
    typedef std::vector<ConnectionSharedPtr> ConnectionList;

    /// The signal host address.
    SignalHost& m_host;
    /// The metasignal of the signal.
    const MetaSignal& m_metaSignal;
    /// The collection of active connections.
    ConnectionList m_connections;
    /// Triggering flag. Locks the signal from recursive triggering.
    bool m_triggering;
};

/// The class is the counterpart of the Mox signals, it holds all the signals declared on a class.
/// Each class that declares signals must be derived from SignalHost class.
class MOX_API SignalHost
{
public:
    /// Destructor.
    virtual ~SignalHost();

    int activate(int signal, Callable::Arguments& args);

protected:
    /// Constructor.
    explicit SignalHost() = default;

    /// Signal host lock, guards the signal register.
    std::mutex m_lock;
    /// The signal register holding all declared signals on a signal host.
    std::vector<const Signal*> m_signals;

    /// Registers the \a signal to the signal host.
    /// \param signal The signal to register.
    void registerSignal(Signal& signal);

    /// Removes a signal from the signal host register.
    /// \param signal The signal to remove from the register.
    void removeSignal(Signal& signal);

    friend class Signal;
    friend class MetaSignal;
};

namespace decl
{

/// Signal template specialization for a generic signature.
template <typename Signature>
class Signal;

/// Signal template, specialization with a signature of void function with arbitrary arguments.
/// The arguments must be registered as metatypes. You can connect methods, metamethods, functions,
/// functors or lambdas to a signal using the relevant connect() methods. You can disconnect the
/// connections either calling disconnect() on the signal, or by calling Signal::Connection::disconnect().
/// \note You can connect signals to slots that have the same arguments or less arguments than
/// the signal has. When connecting to slots with less arguments than the signal signature, only
/// slots that have the same argument types at the specific argument index are considered connectable.
/// This is also valid when connecting two signals.
template <typename... Args>
class Signal<void(Args...)> : public mox::Signal
{
    template <class SignalOwner>
    static const MetaSignal& getMetaSignal(std::string_view name);

public:
    /// The signature of the signal.
    typedef void(*Signature)(Args...);

    /// Constructs the signal attaching it to the \a owner.
    /// \param owner The SignalHost owning the signal.
    /// \param name The name of the signal.
    template <class SignalOwner>
    explicit Signal(SignalOwner& owner, std::string_view name);
};

} // namespace impl

/// Emits a \a signal on \a sender with the optional \a arguments. If the signal exists on the sender
/// and is invocable with the given arguments, returns the number of activations of that signal.
/// \param signal The signal name to emit.
/// \param sender The sender object that has the signal.
/// \param arguments The optional arguments to pass to the signal.
/// \return If the signal is not found on the sender, returns -1. If the signal is found, returns the
/// number of activations.
template <class Sender, typename... Args>
auto emit(std::string_view signal, Sender& sender, Args... arguments);

} // mox

#include <mox/metadata/detail/signal_impl.hpp>

#endif // SIGNAL_HPP
