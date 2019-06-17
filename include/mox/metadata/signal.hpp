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
#define META_SIGNAL(name)  mox::MetaSignalImpl<decltype(name)> meta_##name{*this, #name}

namespace mox
{

class Signal;
class SignalHost;

class MOX_API MetaSignal
{
public:
    std::string name() const
    {
        return m_name;
    }
    /// Returns an array with the argument descriptors of the signal.
    const Callable::ArgumentDescriptorContainer arguments() const
    {
        return m_arguments;
    }

    size_t activate(SignalHost& sender, Callable::Arguments& arguments) const;

protected:
    explicit MetaSignal(MetaClass& metaClass, std::string_view name, const Callable::ArgumentDescriptorContainer& args);
    virtual ~MetaSignal() =  default;

    MetaClass& m_ownerClass;
    Callable::ArgumentDescriptorContainer m_arguments;
    std::string m_name;
    size_t m_id;
};

template <class SignalType>
class MetaSignalImpl : public MetaSignal
{
public:
    explicit MetaSignalImpl(MetaClass& metaClass, std::string_view name)
        : MetaSignal(metaClass, name, function_traits<typename SignalType::Signature>::argument_descriptors())
    {
    }
};

/// Signal is the base class of the Mox signals. You can declare signals using the Signal template class.
class MOX_API Signal
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
        explicit Connection(Signal& signal);

        /// Activates the connection by calling the slot of the connection.
        /// \param args The arguments to pass to the slot.
        virtual void activate(Callable::Arguments& args) = 0;

        /// Resets the connection.
        virtual void reset() = 0;

        /// The signal the connection is attached to.
        Signal& m_signal;

        friend class Signal;
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
    ConnectionSharedPtr connect(const Signal& signal);

    /// Activates the connections of the signal by invoking the slots from each connection passing
    /// the \a arguments to the slots. Connections created during the activation are not invoked
    /// in the same activation cicle.
    /// \param arguments The arguments to pass to the slots, being the arguments passed to the signal.
    /// \return The number of connections activated.
    size_t activate(Callable::Arguments& arguments);

    /// Returns the metasignal assiciated to the signal.
    /// \return The metasignal of the signal.
    const MetaSignal& metaSignal() const
    {
        return m_metaSignal;
    }

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
    ConnectionSharedPtr connect(std::any receiver, Callable&& slot);

    /// Disconnects a connection that holds a \a receiver and \a callableAddress.
    bool disconnect(std::any receiver, const void* callableAddress);
    /// Disconnects a connection that holds the \a signal.
    bool disconnect(const Signal& signal);

    /// Connection container type.
    typedef std::vector<ConnectionSharedPtr> ConnectionList;

    /// The signal host address.
    SignalHost& m_host;
    /// The metasignal of the signal.
    const MetaSignal& m_metaSignal;
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
    std::vector<const Signal*> m_signals;

    /// Registers the \a signal to the signal host.
    /// \param signal The signal to register.
    /// \return The signal identifier.
    size_t registerSignal(Signal& signal);

    /// Removes a signal from the signal host register.
    /// \param signal The signal to remove from the register.
    void removeSignal(Signal& signal);

    friend class Signal;
};

/// Signal template specialization for a generic signature.
template <typename Signature>
class SignalType;

/// Signal template, specialization with a signature of void function with arbitrary arguments.
/// The arguments must be registered as metatypes. You can connect methods, metamethods, functions,
/// functors or lambdas to a signal using the relevant connect() methods. You can disconnect the
/// connections either calling disconnect() on the signal, or by calling Signal::Connection::disconnect().
/// \note You can connect signals to slots that have the same arguments or less arguments than
/// the signal has. When connecting to slots with less arguments than the signal signature, only
/// slots that have the same argument types at the specific argument index are considered connectable.
/// This is also valid when connecting two signals.
template <typename... Args>
class SignalType<void(Args...)> : public Signal
{
    template <class SignalOwner>
    static const MetaSignal& getMetaSignal(std::string_view name)
    {
        const std::array<ArgumentDescriptor, sizeof... (Args)> des = {{ ArgumentDescriptor::get<Args>()... }};
        const std::vector<ArgumentDescriptor> desArray(des.begin(), des.end());
        auto visitor = [desArray = std::move(desArray), name = std::string(name)](const MetaSignal* signal)
        {
            return (signal->name() == name) && (signal->arguments() == desArray);
        };
        const MetaSignal* signal = SignalOwner::getStaticMetaClass()->visitSignals(visitor);
        ASSERT(signal, std::string("Cannot create a signal without a metasignal for ") + std::string(name));
        return *signal;
    }

public:
    /// The signature of the signal.
    typedef void(*Signature)(Args...);

    /// Constructs the signal attaching it to the \a owner.
    /// \param owner The SignalHost owning the signal.
    /// \param name The name of the signal.
    template <class SignalOwner>
    explicit SignalType(SignalOwner& owner, std::string_view name)
        : Signal(owner, getMetaSignal<SignalOwner>(name))
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
    typename std::enable_if<std::is_base_of_v<Signal, ReceiverSignal>, ConnectionSharedPtr>::type
    connect(const ReceiverSignal& signal);

    /// Connects a \a function, or a lambda to this signal.
    /// \param function The function, functor or lambda to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename Function>
    typename std::enable_if<!std::is_base_of_v<Signal, Function>, ConnectionSharedPtr>::type
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
    typename std::enable_if<!std::is_base_of_v<Signal, SlotFunction>, bool>::type
    disconnect(const SlotFunction& slot);

    /// Disconnects a \a signal from this signal.
    /// \param signal The signal to disconnect.
    /// \return If the \a signal was connected, and the disconnect succeeded, returns \e true.
    /// Otherwise returns \e false.
    template <typename SignalClass>
    typename std::enable_if<std::is_base_of_v<Signal, SignalClass>, bool>::type
    disconnect(const SignalClass& signal);
};

} // mox

#include <mox/metadata/detail/signal_impl.hpp>

#endif // SIGNAL_HPP
