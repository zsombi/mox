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

#include <mox/config/deftypes.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/containers.hpp>
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metaclass.hpp>
#include <mox/signal/signal_type.hpp>
#include <mox/utils/function_traits.hpp>

namespace mox
{

class SignalType;

/******************************************************************************
 *
 */

/// Signal defines the signals in your class, and holds the connections made against the
/// signal. You can connect a signal to a method, a metamethod, a function, a functor or
/// a lambda using the specialized connect() functions. The functions connected to a signal
/// are called slots. The slots must have arguments no more than the signal has, and must
/// be of the same type or at least convertible to the slot arguments. If these conditions
/// are not met, the connection fails.
///
/// A signal that is connected to an object from an other thread is marked for asynchrnous
/// processing. This requires that the thread the slot belongs to must have an event loop
/// to schedule the deferred slot processing.
///
/// When the object to which the signal belongs is destroyed, all the signal connections are
/// also disconnected. All asynchronous connections are marked as invalid, so when scheduled,
/// those will not get processed.
///
/// Use SignalDecl<> template to declare your signals with specialized arguments.
class MOX_API Signal : public SharedLock<ObjectLock>
{
    friend class SignalType;
    friend class Property;

public:
    class Connection;
    /// The connection type.
    using ConnectionSharedPtr = std::shared_ptr<Connection>;

    /// The class represents a connection to a signal. The connection is a token which holds
    /// the signal connected, and the slot the signal is connected to. The slot is a function,
    /// a method, a metamethod, a functor or a lambda.
    class MOX_API Connection : public std::enable_shared_from_this<Connection>
    {
    public:
        /// Destructor.
        virtual ~Connection() = default;

        /// Returns the state of the connection.
        /// \return If the connection is connected, \e true, otherwise \e false.
        virtual bool isConnected() const = 0;

        /// Returns the sender signal of the connection.
        /// \return The sender signal of the connection, \e nullptr if the signal is destroyed
        /// before the connection gets activated.
        Signal* signal() const;

        /// Disconnects the signal.
        /// \return If the disconnect succeeds, \e true. If the disconnect fails, \e false.
        bool disconnect();

        /// Returns the active connection that activated your slot. You should call this method
        /// to access the connection and its properties.
        /// \return The active connection object.
        static ConnectionSharedPtr getActiveConnection();

    protected:
        /// Constructs a connection attached to the \a signal.
        explicit Connection(Signal& signal);

        /// Activates the connection by calling the slot of the connection.
        /// \param args The arguments to pass to the slot.
        virtual void activate(const Callable::ArgumentPack& args) = 0;

        /// Resets the connection.
        virtual void reset();

        /// Internal disconnect method, to disconnect a connection specific receiver.
        /// \return If the connection's receiver matches the one passed as argument,
        /// return tru, otherwise false.
        virtual bool disconnect(Variant receiver, const Callable& callable) = 0;

        /// The signal the connection is attached to.
        Signal* m_signal = nullptr;

        friend class Signal;
        friend class DeferredSignalEvent;
    };

    /// Returns the signal type.
    /// \return The signal type.
    SignalType* getType();
    const SignalType* getType() const;

    /// Activates the connections of the signal by invoking the slots from each connection passing
    /// the \a arguments to the slots. Connections created during the activation are not invoked
    /// in the same activation cicle.
    /// \param arguments The arguments to pass to the slots, being the arguments passed to the signal.
    /// \return The number of connections activated.
    int activate(const Callable::ArgumentPack& arguments);

    /// Creates a connection between a signal and a \a metaMethod of a \a receiver.
    /// \param receiver The receiver hosting the metamethod.
    /// \param metaMethod The metamethod to connect to.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(Variant receiver, const MetaClass::Method& metaMethod);

    /// Creates a connection between this signal and a receiver \a signal.
    /// \param signal The receiver signal connected to this signal.
    /// \return The connection shared object.
    ConnectionSharedPtr connect(const Signal& signal);
    /// Disconnects a \a signal from this signal.
    /// \param signal The signal to disconnect.
    /// \return If the \a signal was connected, and the disconnect succeeded, returns \e true.
    /// Otherwise returns \e false.
    bool disconnect(const Signal& signal);

    /// Connects a metamethod with \a methodName. The metamethod must be registered in the \a receiver's
    /// static or dynamic metaclass.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <class Receiver>
    ConnectionSharedPtr connect(const Receiver& receiver, const char* methodName);
    /// Disconnects a metamethod with \a methodName. The metamethod must be registered in the \a receiver's
    /// static or dynamic metaclass.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the method with the \a methodName and \a receiver was connected, and the disconnect succeeded,
    /// returns \e true. Otherwise returns \e false.
    template <typename Receiver>
    bool disconnect(const Receiver& receiver, const char* methodName);

    /// Connects a \a method of a \a receiver thos this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename SlotFunction>
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionSharedPtr>
    connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);
    /// Disconnects a \a method that is a method of the \a receiver.
    /// \param receiver The receiver of the connection.
    /// \param methodName The name of the metamethod to connect.
    /// \return If the method with the \a methodName and \a receiver was connected, and the disconnect
    /// succeeded, returns \e true. Otherwise returns \e false.
    template <typename SlotFunction>
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, bool>
    disconnect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return If the connection succeeds, returns the shared pointer to the connection. If the connection
    /// fails, returns \e nullptr.
    template <typename Function>
    std::enable_if_t<!std::is_base_of_v<mox::Signal, Function>, ConnectionSharedPtr>
    connect(const Function& slot);
    /// Disonnects a \a function, functor or a lambda from this signal.
    /// \param function The function, functor or lambda to disconnect.
    /// \return If the \a function, functor or lambda was connected, and the disconnect succeeded,
    /// returns \e true. Otherwise returns \e false.
    template <typename Function>
    std::enable_if_t<!std::is_base_of_v<Signal, Function>, bool>
    disconnect(const Function& slot);

    /// Destructor.
    virtual ~Signal();

protected:
    Signal() = delete;
    DISABLE_COPY(Signal)

    /// Construct the
    explicit Signal(intptr_t owner, SignalType& signalType);

    /// Adds a \a connection to the signal.
    void addConnection(ConnectionSharedPtr connection);
    /// Removes a \a connection from the signal.
    void removeConnection(ConnectionSharedPtr connection);

    /// Creates a connection to a \a lambda. The connection owns the callable.
    ConnectionSharedPtr connect(Callable&& lambda);
    /// Creates a connection to a \a receiver and a \a slot. The connection owns the callable.
    ConnectionSharedPtr connect(Variant receiver, Callable&& slot);

    /// Disconnects a connection that holds a \a receiver and \a callableAddress.
    bool disconnectImpl(Variant receiver, const Callable& callable);


    /// The collection of active connections.
    using ConnectionContainer = SharedVector<ConnectionSharedPtr>;
    ConnectionContainer m_connections;
    /// The signal type.
    SignalType* m_signalType = nullptr;
    /// The signal owner.
    intptr_t m_owner = 0;
    /// Triggering flag. Locks the signal from recursive triggering.
    bool m_triggering = false;
};

template <typename... Arguments>
class SignalDecl : public Signal
{
public:
    template <class SignalOwner>
    explicit SignalDecl(SignalOwner& owner, SignalType& type)
        : Signal(reinterpret_cast<intptr_t>(&owner), type)
    {
        // Signal arguments must match with the signal type.
        auto signalArgs = VariantDescriptorContainer::get<Arguments...>();
        FATAL(type.getArguments() == signalArgs, "Signal arguments and signal type arguments mismatch")
    }

    /// Signal emitter. Packs the \a arguments into a Callable::ArgumentPack pack and activates
    /// the signal connections.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections activated.
    int operator()(Arguments... arguments);
};

} // mox

#include <mox/signal/detail/signal_impl.hpp>

#endif // SIGNAL_HPP
