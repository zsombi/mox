// Copyright (C) 2020 bitWelder

#include <mox/core/metakernel/properties.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/core/metakernel/signal_connection.hpp>
#include <mox/utils/algorithm.hpp>
#include <mox/utils/locks.hpp>

namespace mox { namespace metakernel {

static ConnectionPtr s_currentConnection;

/******************************************************************************
 * Connection
 */
Connection::Connection(SignalCore& sender)
    : m_sender(&sender)
{
}

Connection::~Connection()
{
    if (isConnected())
    {
        disconnect();
    }
}

bool Connection::isConnected() const
{
    return m_sender != nullptr;
}

void Connection::disconnect()
{
    m_sender->disconnect(shared_from_this());
    invalidate();
}

void Connection::invalidate()
{
    if (!m_sender)
    {
        // Already invalidated.
        return;
    }
    m_sender = nullptr;
    invalidateOverride();
}

void Connection::invoke(const PackedArguments& arguments)
{
    struct ConnectionScope
    {
        ConnectionPtr prev;
        ConnectionScope(Connection& connection)
            : prev(s_currentConnection)
        {
            s_currentConnection = connection.shared_from_this();
        }
        ~ConnectionScope()
        {
            s_currentConnection = prev;
        }
    } scope(*this);

    invokeOverride(arguments);
}

ConnectionPtr Connection::getActiveConnection()
{
    return s_currentConnection;
}

SignalCore* Connection::getSignal() const
{
    return m_sender;
}

/******************************************************************************
 * SlotHolder
 */
SlotHolder::~SlotHolder()
{
    auto invalidator = [](auto& connection)
    {
        if (connection && connection->isConnected())
        {
            connection->invalidate();
        }
    };
    for_each(m_slots, invalidator);
}

void SlotHolder::addConnection(ConnectionPtr connection)
{
    FATAL(connection, "Invalid connection");
    m_slots.push_back(connection);
}

void SlotHolder::removeConnection(ConnectionPtr connection)
{
    FATAL(connection, "Invalid connection");
    erase(m_slots, connection);
}


void SlotHolder::disconnectAll()
{
    auto disconnector = [](auto& connection)
    {
        if (connection && connection->isConnected())
        {
            connection->disconnect();
        }
    };
    for_each(m_slots, disconnector);
}

/******************************************************************************
 * SignalCore
 */
SignalCore::SignalCore(size_t argCount)
    : m_argumentCount(argCount)
{
}

SignalCore::~SignalCore()
{
    lock_guard guard(m_connections);
    auto disconnector = [self = this](auto& connection)
    {
        if (connection && connection->isConnected())
        {
            self->disconnect(connection);
        }
    };
    for_each(m_connections, disconnector);
}

size_t SignalCore::getArgumentCount() const
{
    return m_argumentCount;
}

int SignalCore::activate(const PackedArguments &args)
{
    if (m_isActivated || m_isBlocked)
    {
        return 0;
    }

    decltype(m_connections)::ContainerType connectionsCopy;
    int activationCount = -1;
    {
        // Lock only till we copy the connections.
        lock_guard lock(*this);
        if (m_connections.empty())
        {
            return activationCount;
        }
        connectionsCopy = m_connections;
    }
    ScopeValue triggerLock(m_isActivated, true);

    auto activateConnection = [&args, &activationCount](auto& connection)
    {
        if (!connection || !connection->isConnected())
        {
            return;
        }

        connection->invoke(args);
        ++activationCount;
    };
    for_each(connectionsCopy, activateConnection);

    if (activationCount >= 0)
    {
        ++activationCount;
    }
    return activationCount;
}

ConnectionPtr SignalCore::connectBinding(BindingCore& binding)
{
    using SlotFunction = decltype(&BindingCore::evaluate);
    auto connection = MethodConnection<SlotFunction>::create(*this, binding, &BindingCore::evaluate);
    addConnection(connection);

    auto holder = dynamic_cast<SlotHolder*>(&binding);
    if (holder)
    {
        holder->addConnection(connection);
    }
    return connection;
}

void SignalCore::disconnect(ConnectionPtr connection)
{
    throwIf<ExceptionType::InvalidArgument>(!connection);
    throwIf<ExceptionType::Disconnected>(!connection->isConnected());

    lock_guard lock(*this);
    erase(m_connections, connection);
    connection->invalidate();
}

void SignalCore::addConnection(ConnectionPtr connection)
{
    FATAL(connection, "Attempt adding a null connection");

    lock_guard lock(*this);
    m_connections.push_back(connection);
}

bool SignalCore::isBlocked() const
{
    return m_isBlocked;
}
void SignalCore::setBlocked(bool block)
{
    m_isBlocked = block;
}

}} // mox::metakernel
