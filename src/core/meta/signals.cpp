// Copyright (C) 2020 bitWelder

#include <private/property_p.hpp>

#include <mox/core/meta/properties.hpp>
#include <mox/core/meta/signals.hpp>
#include <mox/core/meta/signal_connection.hpp>
#include <mox/utils/algorithm.hpp>
#include <mox/utils/locks.hpp>

namespace mox
{

static thread_local ConnectionPtr s_currentConnection;

/******************************************************************************
 * ConnectionStorage
 */
void ConnectionStorage::disconnectSlots()
{
    P();
    p->m_isActivated = true;
    p->m_isBlocked = true;

    lock_guard lockSignal(*p);
    lock_guard lockConnections(connections);

    auto disconnector = [p](auto& connection)
    {
        if (!connection || !connection->isConnected())
        {
            return;
        }

        auto slot = connection->getDestination();
        if (slot)
        {
            lock_guard lockSlot(*slot);
            UNUSED(p);
//            OrderedRelock<Lockable*> re(p, slot);
            erase(slot->m_slots, connection);
        }
        connection->m_sender = nullptr;
        connection->invalidateOverride();
    };
    for_each(connections, disconnector);
}

void ConnectionStorage::disconnectOne(ConnectionPtr connection)
{
    P();
    lock_guard lockSignal(*p);

    auto slot = connection->getDestination();
    if (slot)
    {
        lock_guard lockSlot(*slot);
//        OrderedRelock<Lockable*> re(p, slot);
        erase(slot->m_slots, connection);
    }
    connection->m_sender = nullptr;
    connection->invalidateOverride();
    erase(connections, connection);
}

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

SlotHolder* Connection::getDestination() const
{
    return nullptr;
}

/******************************************************************************
 * SlotHolder
 */
SlotHolder::~SlotHolder()
{
    disconnectSignals();
}

void SlotHolder::addConnection(ConnectionPtr connection)
{
    FATAL(connection, "Invalid connection");
    lock_guard lock(*this);
    m_slots.push_back(connection);
}

void SlotHolder::removeConnection(ConnectionPtr connection)
{
    FATAL(connection, "Invalid connection");
    lock_guard lock(*this);
    erase(m_slots, connection);
}


void SlotHolder::disconnectSignals()
{
    lock_guard lock(*this);
    auto disconnector = [self = this](auto& connection)
    {
        if (!connection || !connection->isConnected())
        {
            return;
        }

        auto sender = ConnectionStorage::get(*connection->getSignal());
        ScopeRelock reLock(*self);
        sender->disconnectOne(connection);
    };
    for_each(m_slots, disconnector);
}

/******************************************************************************
 * SignalCore
 */
SignalCore::SignalCore(Lockable& host, size_t argCount)
    : d_ptr(pimpl::make_d_ptr<ConnectionStorage>(*this))
    , m_argumentCount(argCount)
{
    UNUSED(host);
}

SignalCore::~SignalCore()
{
    d_ptr->disconnectSlots();
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

    decltype(ConnectionStorage::connections)::ContainerType connectionsCopy;
    int activationCount = -1;
    {
        // Lock only till we copy the connections.
        lock_guard lock(*this);
        if (d_ptr->connections.empty())
        {
            return activationCount;
        }
        connectionsCopy = d_ptr->connections;
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

    d_ptr->disconnectOne(connection);
}

void SignalCore::addConnection(ConnectionPtr connection)
{
    FATAL(connection, "Attempt adding a null connection");

    lock_guard lock(*this);
    d_ptr->connections.push_back(connection);
}

bool SignalCore::isBlocked() const
{
    return m_isBlocked;
}
void SignalCore::setBlocked(bool block)
{
    m_isBlocked = block;
}

} // mox
