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

#include <mox/core/event_handling/socket_notifier.hpp>
#include <mox/core/process/thread_data.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <process_p.hpp>

namespace mox
{

SocketNotifier::SocketNotifier(EventTarget handler, Modes modes)
    : SocketNotifierSource::Notifier(handler, modes)
{
}

SocketNotifier::~SocketNotifier()
{
    setEnabled(false);
}

void SocketNotifier::signal(Modes mode)
{
    auto self = std::static_pointer_cast<SocketNotifier>(shared_from_this());
    activated(self, mode);
}

SocketNotifierSharedPtr SocketNotifier::create(EventTarget socket, Modes modes)
{
    SocketNotifierSharedPtr notifier(new SocketNotifier(socket, modes));
    notifier->setEnabled(true);
    return notifier;
}

bool SocketNotifier::isEnabled() const
{
    return !m_source.expired();
}

void SocketNotifier::setEnabled(bool enabled)
{
    auto source = m_source.lock();
    if (!source && enabled)
    {
        auto thread = ThreadData::getThisThreadData()->thread();
        auto d = ThreadInterfacePrivate::get(*thread);
        source = d->runLoop->getDefaultSocketNotifierSource();
        attach(*source);
    }
    else if (source && !enabled)
    {
        detach();
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
