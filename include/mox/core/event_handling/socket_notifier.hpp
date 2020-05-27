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

#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/core/event_handling/event_handling_declarations.hpp>

namespace mox
{

/// The SocketNotifier class provides notifications on events occurring on a socket descriptor.
/// The socket descriptor is either a file handler or a normal socket handler.
class MOX_API SocketNotifier : public metakernel::Lockable, public metakernel::SlotHolder, public SocketNotifierSource::Notifier
{
public:
    /// Activation signal, emitted when the operation on the socket is notified.
    metakernel::Signal<SocketNotifierSharedPtr, Modes> activated{*this};

    /// Creates a socket notifier on a \a socket with notification \a modes.
    static SocketNotifierSharedPtr create(EventTarget socket, Modes modes);
    /// Destructor.
    ~SocketNotifier();

    /// Enables or disables the socket notifier.
    void setEnabled(bool enabled);
    /// Returns the enabled state of the notifier.
    bool isEnabled() const;

    /// Returns \e true if this socket notifier is watching for read mode.
    bool hasReadMode() const;
    /// Returns \e true if this socket notifier is watching for write mode.
    bool hasWriteMode() const;
    /// Returns \e true if this socket notifier is watching for exception mode.
    bool hasErrorMode() const;
    /// Returns \e true if this socket notifier is watching for error mode.
    bool hasExceptionMode() const;

protected:
    /// Constructor.
    explicit SocketNotifier(EventTarget handler, Modes modes);

    void signal(Modes mode) override;
};

}

#endif // SOCKET_NOTIFIER_HPP
