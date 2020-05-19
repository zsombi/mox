// Copyright (C) 2020 bitWelder

#include <mox/core/metakernel/signals.hpp>
#include <mox/core/metakernel/signal_connection.hpp>
#include <mox/utils/algorithm.hpp>
#include <mox/utils/locks.hpp>

namespace mox { namespace metakernel {

static thread_local std::stack<ConnectionPtr> s_threadConnectionStack;

/******************************************************************************
 * Connection
 */
Connection::Connection(SignalCore& sender)
    : m_sender(&sender)
{
}

Connection::~Connection()
{
}

bool Connection::isConnected() const
{
    return m_sender != nullptr;
}

void Connection::disconnect()
{
    // TODO: lock sender!
    m_sender->disconnect(shared_from_this());
    invalidate();
}

void Connection::invalidate()
{
    m_sender = nullptr;
    invalidateOverride();
}

void Connection::invoke(const PackedArguments& arguments)
{
    struct ConnectionScope
    {
        ConnectionScope(Connection& connection)
        {
            s_threadConnectionStack.push(connection.shared_from_this());
        }
        ~ConnectionScope()
        {
            s_threadConnectionStack.pop();
        }

    } scope(*this);
    invokeOverride(arguments);
}

ConnectionPtr Connection::getActiveConnection()
{
    return s_threadConnectionStack.top();
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

/******************************************************************************
 * SignalCore
 */
SignalCore::SignalCore(size_t argCount)
    : m_argumentCount(argCount)
{
}

SignalCore::~SignalCore()
{
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

    lock_guard lock(*this);
    if (m_connections.empty())
    {
        return -1;
    }
    ScopeValue triggerLock(m_isActivated, true);
    int activationCount = -1;

    // create a copy of the connections, as it may change during activation time.
    auto connectionCopy = m_connections;

    auto activateConnection = [&args, self = this, &activationCount](auto& connection)
    {
        if (!connection || !connection->isConnected())
        {
            return;
        }

        ScopeRelock re(*self);
        connection->invoke(args);
        ++activationCount;
    };
    for_each(connectionCopy, activateConnection);

    if (activationCount >= 0)
    {
        ++activationCount;
    }
    return activationCount;
}

void SignalCore::disconnect(ConnectionPtr connection)
{
    throwIf<ExceptionType::InvalidArgument>(!connection);
    throwIf<ExceptionType::Disconnected>(!connection->isConnected());

    lock_guard lock(*this);
    mox::erase(m_connections, connection);
    connection->invalidate();
}

void SignalCore::addConnection(ConnectionPtr connection)
{
    FATAL(connection, "Attempt adding a null connection");

    lock_guard lock(*this);
    m_connections.push_back(connection);
}

}} // mox::metakernel
