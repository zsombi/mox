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
#include <type_traits>
#include <vector>
#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metaclass.hpp>

#include <mox/signal/signal_host.hpp>
#include <mox/utils/function_traits.hpp>

namespace mox
{

class Signal;
class SignalHostNotion;

/// Abstract signal descriptor is the base descriptor class of mox signal types. The signal
/// descriptor holds the argument signatures (descriptors) of a signal, and a unique identifier
/// associated to the signal type.
///
/// You can have mox signals in your class if:
/// - you declare a signal descriptor for the signal as static inline const member,
/// - you derive your class from SignalHostNotion or from a class that derives from SignalHostNotion.
struct MOX_API AbstractSignalDescriptor
{
    typedef int64_t TUuid;

    /// Holds the signal argument descriptors.
    const VariantDescriptorContainer arguments;
    /// Holds the unique identifier of the signal.
    const TUuid uuid = 0u;

    // Comparison operator.
    bool operator==(const AbstractSignalDescriptor& other) const;

protected:
    /// Construct the signal descriptor with the argument container passed.
    AbstractSignalDescriptor(const VariantDescriptorContainer& arguments);
};

/// Signal is the concept of the Mox signals.
///
/// You can connect a signal to a method, a metamethod, a function, a functor or a lambda using the specialized
/// connect() functions. The functions connected to a signal are called slots. The slots must have arguments no
/// more than the signal has, and must be of the same type or at least convertible to the slot arguments. If the
/// conditions are not met, the connection fails.
///
/// Optionally, a slot can have the first argument of type Signal::ConnectionSharedPtr. When so, the connection
/// object is passed to the slot when the signal is activated. The slot can use the connection object to disconnect
/// the slot from the signal.
///
/// Every mox signal is registered to a host. When the host is destroyed, all the signal connections are also
/// disconnected and deferred.
class MOX_API Signal : public ObjectLock
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
        virtual bool compare(Variant receiver, const void* funcAddress) const;

    protected:
        /// Constructs a connection attached to the \a signal.
        explicit Connection(Signal& signal);

        /// Activates the connection by calling the slot of the connection.
        /// \param args The arguments to pass to the slot.
        virtual void activate(Callable::ArgumentPack& args) = 0;

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

    /// Returns the signal host instance.
    /// \return The signal host instance.
    SignalHostNotion& host() const;

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
    int activate(Callable::ArgumentPack& arguments);

    /// Returns the signal descriptor.
    const AbstractSignalDescriptor& descriptor() const
    {
        return m_descriptor;
    }

    /// Signal emitter. Packs the \a arguments into a Callable::ArgumentPack pack and activates
    /// the signal connections.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections activated.
    template <typename... Args>
    int operator()(Args... arguments);

    /// Creates a connection between a signal and a \a metaMethod of a \a receiver.
    /// \param receiver The receiver hosting the metamethod.
    /// \param metaMethod The metamethod to connect to.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(Variant receiver, const MetaClass::Method& metaMethod);

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

    template <class SignalOwner>
    explicit Signal(SignalOwner& owner, const AbstractSignalDescriptor& des)
        : m_host(owner)
        , m_descriptor(des)
        , m_id(m_host.registerSignal(*this))
    {
    }
    /// Destructor.
    virtual ~Signal();

protected:
    Signal() = delete;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// Adds a \a connection to the signal.
    void addConnection(ConnectionSharedPtr connection);
    /// Removes a \a connection from the signal.
    void removeConnection(ConnectionSharedPtr connection);

    /// Creates a connection to a \a lambda. The connection owns the callable.
    ConnectionSharedPtr connect(Callable&& lambda);
    /// Creates a connection to a \a receiver and a \a slot. The connection owns the callable.
    ConnectionSharedPtr connect(Variant receiver, Callable&& slot);

    /// Disconnects a connection that holds a \a receiver and \a callableAddress.
    bool disconnectImpl(Variant receiver, const void* callableAddress);

    /// Connection container type.
    typedef std::vector<ConnectionSharedPtr> ConnectionList;

    /// The signal host address.
    SignalHostNotion& m_host;
    /// The signal descriptor.
    const AbstractSignalDescriptor& m_descriptor;
    /// The metasignal of the signal.
    size_t m_id = std::numeric_limits<size_t>::max();
    /// The collection of active connections.
    ConnectionList m_connections;
    /// Triggering flag. Locks the signal from recursive triggering.
    bool m_triggering = false;
};

template <typename... ArgumentPack>
struct SignalDescriptor : AbstractSignalDescriptor
{
    SignalDescriptor()
        : AbstractSignalDescriptor(VariantDescriptorContainer::get<ArgumentPack...>())
    {
    }
};

} // mox

#include <mox/signal/detail/signal_impl.hpp>

#endif // SIGNAL_HPP
