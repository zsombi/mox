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

#include "event_dispatcher.h"

namespace mox
{

SocketNotifierCore::Modes Adaptation::supportedModes()
{
    return SocketNotifierCore::Modes::Read 
        | SocketNotifierCore::Modes::Write 
        | SocketNotifierCore::Modes::Error 
        | SocketNotifierCore::Modes::Exception;
}

/******************************************************************************
 * GSocketNotifierSource::Source
 */
namespace
{

constexpr unsigned short readMask = G_IO_IN | G_IO_HUP;
constexpr unsigned short writeMask = G_IO_OUT;
constexpr unsigned short exceptionMask = G_IO_PRI;
constexpr unsigned short errorMask = G_IO_ERR;

constexpr bool pollRead(SocketNotifierCore::Modes modes)
{
    return (modes & SocketNotifierCore::Modes::Read) == SocketNotifierCore::Modes::Read;
}
constexpr bool pollWrite(SocketNotifierCore::Modes modes)
{
    return (modes & SocketNotifierCore::Modes::Write) == SocketNotifierCore::Modes::Write;
}
constexpr bool pollException(SocketNotifierCore::Modes modes)
{
    return (modes & SocketNotifierCore::Modes::Exception) == SocketNotifierCore::Modes::Exception;
}
constexpr bool pollError(SocketNotifierCore::Modes modes)
{
    return (modes & SocketNotifierCore::Modes::Error) == SocketNotifierCore::Modes::Error;
}

} // noname

GPollHandler::GPollHandler(SocketNotifierCore& notifier)
{
    this->notifier = notifier.shared_from_this();
    fd.fd = notifier.handler();
    fd.events = 0;

    auto modes = notifier.getModes();
    if (pollRead(modes))
    {
        fd.events |= readMask | G_IO_ERR;
    }
    if (pollWrite(modes))
    {
        fd.events |= writeMask | G_IO_ERR;
    }
    if (pollException(modes))
    {
        fd.events |= exceptionMask | G_IO_ERR;
    }
}

void GPollHandler::reset()
{
    notifier.reset();
    fd.fd = -1;
    fd.events = fd.revents = 0;
}

GlibRunLoopBase::SocketNotifierSource* GlibRunLoopBase::SocketNotifierSource::create(GlibRunLoopBase& self, GMainContext* context)
{
    static GSourceFuncs funcs =
    {
        SocketNotifierSource::prepare,
        SocketNotifierSource::check,
        SocketNotifierSource::dispatch,
        nullptr,
        nullptr,
        nullptr
    };

    auto source = static_cast<SocketNotifierSource*>(g_source_new(&funcs, sizeof(SocketNotifierSource)));
    source->m_self = &self;
    auto src = static_cast<GSource*>(source);
//    g_source_set_can_recurse(src, true);
    g_source_attach(src, context);

    return source;
}

void GlibRunLoopBase::SocketNotifierSource::destroy(SocketNotifierSource*& source)
{
    if (!source)
    {
        return;
    }

    auto src = static_cast<GSource*>(source);
    // auto cleanup = [src](GPollHandler& handler)
    // {
    //     g_source_remove_poll(src, &handler.fd);
    //     handler.reset();
    // };
    // for_each(source->pollHandlers, cleanup);

    CTRACE(event, "detach SocketNotifier runloop source");
    auto close = [](const GPollHandler& handler)
    {
        if (handler.notifier)
        {
            handler.notifier->detach();
        }
    };
    for_each(source->m_pollHandlers, close);
    source->m_self = nullptr;

    g_source_destroy(src);
    g_source_unref(src);
    source = nullptr;

    CTRACE(event, "socket source destroyed");
}

gboolean GlibRunLoopBase::SocketNotifierSource::prepare(GSource*, gint* timeout)
{
    //we can not yet determine if the GSource is ready, polling FDs also have no
    //timeout, so lets continue
    if (timeout)
    {
        *timeout = -1;
    }
    return false;
}

//here we need to figure out which FDs are pending
gboolean GlibRunLoopBase::SocketNotifierSource::check(GSource* src)
{
    auto* source = static_cast<SocketNotifierSource*>(src);
    if (!source->m_self)
    {
        CWARN(event, "Orphan socket notifier source invoked!");
        return false;
    }

    //check for pending and remove orphaned entries
    bool hasPendingEvents = false;

    auto callback = [&hasPendingEvents](GPollHandler& poll)
    {
        if (poll.fd.revents & G_IO_NVAL)
        {
            // Detach the invalid socket notifier. This will at least reset the poll.
            poll.notifier->detach();
        }
        else
        {
            hasPendingEvents = hasPendingEvents || ((poll.fd.revents & poll.fd.events) != 0);
        }
    };
    for_each(source->m_pollHandlers, callback);

    // if the pollfds are empty trigger dispatch so this source can be removed
    return hasPendingEvents;
}

//Trigger all event sources that have been activated
gboolean GlibRunLoopBase::SocketNotifierSource::dispatch(GSource *src, GSourceFunc, gpointer)
{
    auto* source = static_cast<SocketNotifierSource*>(src);
    if (!source)
    {
        return G_SOURCE_REMOVE;
    }
    if (!source->m_self)
    {
        CWARN(event, "Orphan socket notifier source invoked!");
        return G_SOURCE_REMOVE;
    }

    auto callback = [](GPollHandler& poll)
    {
        if ((poll.fd.revents & poll.fd.events) != 0 && poll.notifier)
        {
            SocketNotifierCore::Modes event = SocketNotifierCore::Modes::Inactive;
            SocketNotifierCore::Modes reqEvents = poll.notifier->getModes();
            if ((poll.fd.revents & readMask) && (pollRead(reqEvents)))
            {
                event = SocketNotifierCore::Modes::Read;
            }
            if ((poll.fd.revents & writeMask) && (pollWrite(reqEvents)))
            {
                event |= SocketNotifierCore::Modes::Write;
            }
            if ((poll.fd.revents & exceptionMask) && (pollException(reqEvents)))
            {
                event |= SocketNotifierCore::Modes::Exception;
            }
            if ((poll.fd.revents & errorMask) && (pollError(reqEvents)))
            {
                event |= SocketNotifierCore::Modes::Error;
            }
            if (event != SocketNotifierCore::Modes::Inactive)
            {
                poll.notifier->signal(event);
            }
        }
    };
    for_each(source->m_pollHandlers, callback);

    return G_SOURCE_CONTINUE;
}

}
