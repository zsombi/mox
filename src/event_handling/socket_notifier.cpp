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

#include <mox/event_handling/socket_notifier.hpp>
#include <mox/module/thread_data.hpp>

namespace mox
{

SocketNotifier::SocketNotifier(Handler handler, Modes modes)
    : m_source(std::dynamic_pointer_cast<SocketNotifierSource>(ThreadData::thisThreadData()->eventDispatcher()->findEventSource("default_socket_notifier")))
    , m_handler(handler)
    , m_modes(modes & supportedModes())
{
}

SocketNotifier::~SocketNotifier()
{
    setEnabled(false);
}

SocketNotifierSharedPtr SocketNotifier::create(Handler socket, Modes modes)
{
    SocketNotifierSharedPtr notifier(new SocketNotifier(socket, modes));
    notifier->setEnabled(true);
    return notifier;
}

void SocketNotifier::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
    {
        return;
    }
    m_enabled = enabled;
    SocketNotifierSourcePtr source = m_source.lock();
    if (!source)
    {
        return;
    }
    if (m_enabled)
    {
        source->addNotifier(*this);
    }
    else
    {
        source->removeNotifier(*this);
    }
}

bool SocketNotifier::hasReadMode() const
{
    return (m_modes & Modes::Read) == Modes::Read;
}

bool SocketNotifier::hasWriteMode() const
{
    return (m_modes & Modes::Write) == Modes::Write;
}

bool SocketNotifier::hasErrorMode() const
{
    return (m_modes & Modes::Error) == Modes::Error;
}

bool SocketNotifier::hasExceptionMode() const
{
    return (m_modes & Modes::Exception) == Modes::Exception;
}

}
