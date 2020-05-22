// Copyright (C) 2020 bitWelder

#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <mox/config/deftypes.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/metakernel/argument_data.hpp>
#include <mox/core/metakernel/lockable.hpp>

namespace mox { namespace metakernel {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

/// The SignalCore defines the core functionality of a Mox signal. It holds the slots connected,
/// the activation and the blocked state of a signal. Use the Signal<> template to declare a signal
/// in your application.
///
/// A generic signal is activated using the activate() method. When a signal is activated, its
/// connections are invoked. Connections created within connections are left out from the signal
/// activation.
class MOX_API SignalCore : public Lockable
{
public:
    /// Destructor.
    ~SignalCore();

    /// Get the argument count of the signal.
    /// \return The argument count of the signal.
    size_t getArgumentCount() const;

    /// Activates the connections of the signal by invoking the slots from each connection passing
    /// the \a arguments to the slots. Connections created during the activation are not invoked
    /// in the same activation cycle.
    /// \param arguments The arguments to pass to the slots, being the arguments passed to the
    /// signal.
    /// \return The number of connections activated, -1 if the signal has no connections, and 0
    /// if the signal is already activated.
    int activate(const PackedArguments& args);

    /// Disconnects a connection from a signal. Removes the \a connection from the connections
    /// of the signal.
    /// \param connection The connection to remove.
    /// \throws ExceptionType::Disconnected when the signal is already disconnected from the
    /// connection passed as argument.
    void disconnect(ConnectionPtr connection);

    /// Adds a \a connection to the signal. You must check the connection argument compatibility
    /// before adding a connection.
    /// \param connection The connection to add.
    void addConnection(ConnectionPtr connection);

    /// Returns the blocked state of a signal.
    /// \return If the signal is blocked, returns \e true, otherwise \e false.
    bool isBlocked() const;
    /// Sets the blocked state of a signal.
    /// \param block The new blocked state of the signal.
    void setBlocked(bool block);

protected:
    explicit SignalCore(size_t argCount);

    using ConnectionContainer = std::vector<ConnectionPtr>;

    ConnectionContainer m_connections;
    const size_t m_argumentCount;
    std::atomic_bool m_isActivated {false};
    std::atomic_bool m_isBlocked {false};
};

/// The Connection class implements a generic connection slot to a signal. The connection is a
/// token which holds the signal connected, and the slot the signal is connected to. The slot
/// is a function, a method, a lambda or an other signal.
///
/// You can disconnect a connection calling the disconnect() or the invalidate() method on the
/// connection. Note that the connection is removed from the sender signal only if the connection
/// is disconnected. Invalidating the connection keeps it on the sender signal, but marks it as
/// disconnected.
///
/// When you connect methods of classes to a signal, make sure those connections are invalidated
/// by the receiver object when the object is destroyed. You can also use SlotHolder to invalidate
/// the connected signals to your class methods.
///
/// Mox connections are invoked synchronously. If your slot resides in a different thread, it is
/// your responsibility to lock the slot, or execute an asynchronous invocation.
class MOX_API Connection : public std::enable_shared_from_this<Connection>
{
public:
    /// Destructor.
    virtual ~Connection();

    /// Checks whether the connection is connected to a sender.
    /// \return The connection status: \e true when connected, \e false when not.
    bool isConnected() const;

    /// Disconnects the sender signal from this connection and invalidates the sconnection.
    void disconnect();

    /// Invalidates the connection. When called, the connection will report being disconnected.
    /// Similar to disconnect, except that it does not remove the connection from the sender
    /// signal.
    void invalidate();

    /// Activates a connection by calling the slot of the connection.
    /// \param arguments The packed arguments to pass to the slot.
    void invoke(const PackedArguments& arguments);

    /// Returns the active connection that activated your slot. Call this method within your slot
    /// to access the connection.
    /// \return The active connection object.
    static ConnectionPtr getActiveConnection();

    /// Gets the signal of the connection.
    /// \return The signal of the connection, or \e nullptr if the connection is not connected.
    SignalCore* getSignal() const;

protected:
    /// Constructs a connection object with a \a sender signal.
    Connection(SignalCore& sender);

    /// Overridable method to invoke a connection specific slot.
    virtual void invokeOverride(const PackedArguments& arguments) = 0;
    /// Overridable method, informs the connection about being invalidated.
    virtual void invalidateOverride() {}

    /// The sender signal. Reset to \e nullptr when the connection is disconnected from the
    /// sender signal.
    SignalCore* m_sender = nullptr;
};

/// The SlotHolder class holds the connections created on a receiver object. Derive your class
/// from SlotHolder if you want to automatically clean up the connections from a signal to the
/// methods of a class.
class MOX_API SlotHolder
{
    std::vector<ConnectionPtr> m_slots;
    DISABLE_COPY_OR_MOVE(SlotHolder)

protected:
    /// Constructor.
    SlotHolder() = default;

public:
    /// Destructor.
    ~SlotHolder();

    /// Adds a connection to the receiver.
    void addConnection(ConnectionPtr connection);

    /// Removes a connection from the receiver.
    void removeConnection(ConnectionPtr connection);
};

/// Use this template to declare signals. Connect the functions, lambdas, methods or signals
/// using the appropriate connect functions.
template <class... Arguments>
class Signal : public SignalCore
{
    DISABLE_COPY_OR_MOVE(Signal)
public:
    /// Constructor.
    Signal()
        : SignalCore(sizeof...(Arguments))
    {
    }

    /// Signal emitter. Packs the \a arguments into a PackedArguments and activates the signal.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections invoked.
//    int operator()(Arguments&&... arguments);
    int operator()(Arguments... arguments);

    /// Connects a \a method of a \a receiver to this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return Returns the shared pointer to the connection.
    template <class SlotFunction>
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
    connect(typename function_traits<SlotFunction>::object& receiver, SlotFunction method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return Returns the shared pointer to the connection.
    template <class SlotFunction>
    std::enable_if_t<!std::is_base_of_v<mox::metakernel::SignalCore, SlotFunction>, ConnectionPtr>
    connect(const SlotFunction& slot);

    /// Creates a connection between this signal and a receiver \a signal.
    /// \param signal The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <class... SignalArguments>
    ConnectionPtr connect(Signal<SignalArguments...>& signal);
};

struct ScopeSignalBlocker
{
    explicit ScopeSignalBlocker(SignalCore& signal)
        : m_signal(signal)
        , m_oldBlockedState(m_signal.isBlocked())
    {
        m_signal.setBlocked(true);
    }
    ~ScopeSignalBlocker()
    {
        m_signal.setBlocked(m_oldBlockedState);
    }

private:
    SignalCore& m_signal;
    bool m_oldBlockedState;
};

}} // mox::metakernel

/******************************************************************************
 * Implementation
 */
#include <mox/core/metakernel/signal_connection.hpp>

#endif // SIGNALS_HPP
