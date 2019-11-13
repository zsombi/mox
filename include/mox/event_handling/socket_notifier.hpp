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

#ifndef SOCKET_NOTIFIER_HPP
#define SOCKET_NOTIFIER_HPP

#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/signal/signal.hpp>
#include <mox/event_handling/event_handling_declarations.hpp>

namespace mox
{

/// The SocketNotifier class provides notifications on events occurring on a socket descriptor.
/// The socket descriptor is either a file handler or a normal socket handler.
class MOX_API SocketNotifier : public ObjectLock, public std::enable_shared_from_this<SocketNotifier>
{
public:
    /// The type of the handler.
    typedef int Handler;
    /// The event modes.
    enum class Modes
    {
        /// Inactive.
        Inactiv = 0,
        /// Notify on read-ability.
        Read = 0x01,
        /// Notify on write-ability.
        Write = 0x02,
        /// Notify on exception.
        Exception = 0x04,
        /// Notify on error.
        Error = 0x08
    };

    /// Activation signal type descriptor.
    static inline SignalTypeDecl<SocketNotifierSharedPtr, Modes> ActivatedSignalType;

    /// Activation signal, emitted when the operation on the socket is notified.
    SignalDecl<SocketNotifierSharedPtr, Modes> activated{*this, ActivatedSignalType};

    /// Creates a socket notifier on a \a socket with notification \a modes.
    static SocketNotifierSharedPtr create(Handler socket, Modes modes);
    /// Destructor.
    ~SocketNotifier();

    /// Enables or disables the socket notifier.
    void setEnabled(bool enabled);
    /// Returns the enabled state of the notifier.
    bool isEnabled() const
    {
        return m_enabled;
    }
    /// Returns the event modes of this notifier.
    Modes modes() const
    {
        return m_modes;
    }
    /// Returns the handler watched.
    Handler handler() const
    {
        return m_handler;
    }

    /// Returns \e true if this socket notifier is watching for read mode.
    bool hasReadMode() const;
    /// Returns \e true if this socket notifier is watching for write mode.
    bool hasWriteMode() const;
    /// Returns \e true if this socket notifier is watching for exception mode.
    bool hasErrorMode() const;
    /// Returns \e true if this socket notifier is watching for error mode.
    bool hasExceptionMode() const;

    /// Returns the notification modes the platform is supporting.
    static Modes supportedModes();

protected:
    /// Constructor.
    explicit SocketNotifier(Handler handler, Modes modes);

    /// The socket notifier event source this notifier connects.
    SocketNotifierSourceWeakPtr m_source;
    /// Th esocket handler.
    Handler m_handler = -1;
    /// The notification modes.
    Modes m_modes = Modes::Read;
    /// The enabled state.
    bool m_enabled = false;
};
ENABLE_ENUM_OPERATORS(SocketNotifier::Modes)

}

#endif // SOCKET_NOTIFIER_HPP
