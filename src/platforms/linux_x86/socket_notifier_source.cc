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

SocketNotifierSource::Notifier::Modes SocketNotifierSource::supportedModes()
{
    return Notifier::Modes::Read | Notifier::Modes::Write | Notifier::Modes::Error | Notifier::Modes::Exception;
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

constexpr bool pollRead(SocketNotifierSource::Notifier::Modes modes)
{
    return (modes & SocketNotifierSource::Notifier::Modes::Read) == SocketNotifierSource::Notifier::Modes::Read;
}
constexpr bool pollWrite(SocketNotifierSource::Notifier::Modes modes)
{
    return (modes & SocketNotifierSource::Notifier::Modes::Write) == SocketNotifierSource::Notifier::Modes::Write;
}
constexpr bool pollException(SocketNotifierSource::Notifier::Modes modes)
{
    return (modes & SocketNotifierSource::Notifier::Modes::Exception) == SocketNotifierSource::Notifier::Modes::Exception;
}
constexpr bool pollError(SocketNotifierSource::Notifier::Modes modes)
{
    return (modes & SocketNotifierSource::Notifier::Modes::Error) == SocketNotifierSource::Notifier::Modes::Error;
}

} // noname

static GSourceFuncs socketNotifierSourceFuncs =
{
    GSocketNotifierSource::Source::prepare,
    GSocketNotifierSource::Source::check,
    GSocketNotifierSource::Source::dispatch,
    nullptr,
    nullptr,
    nullptr
};

GPollHandler::GPollHandler(SocketNotifierSource::Notifier& notifier)
{
    this->notifier = notifier.shared_from_this();
    fd.fd = notifier.handler();
    fd.events = 0;

    SocketNotifierSource::Notifier::Modes modes = notifier.getModes();
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

GSocketNotifierSource::Source* GSocketNotifierSource::Source::create(GSocketNotifierSource& socketSource, GMainContext* context)
{
    Source *src = reinterpret_cast<Source*>(g_source_new(&socketNotifierSourceFuncs, sizeof(*src)));
    src->self = as_shared<GSocketNotifierSource>(&socketSource);

    GSource* source = static_cast<GSource*>(src);
//    g_source_set_can_recurse(source, true);
    g_source_attach(source, context);

    return src;
}

void GSocketNotifierSource::Source::destroy(Source*& src)
{
    if (!src)
    {
        return;
    }
    GSource* gsource = static_cast<GSource*>(src);

    g_source_destroy(gsource);
    g_source_unref(gsource);
    src = nullptr;
    CTRACE(event, "socket source destroyed");
}

gboolean GSocketNotifierSource::Source::prepare(GSource*, gint* timeout)
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
gboolean GSocketNotifierSource::Source::check(GSource* source)
{
    Source *src = static_cast<Source*>(source);
    auto rlSource = src->self.lock();
    if (!rlSource)
    {
        CWARN(event, "Orphan socket notifier source invoked!");
        return false;
    }
    if (rlSource->getRunLoop()->isExiting())
    {
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
    for_each(rlSource->pollHandlers, callback);

    // if the pollfds are empty trigger dispatch so this source can be removed
    return hasPendingEvents;
}

//Trigger all event sources that have been activated
gboolean GSocketNotifierSource::Source::dispatch(GSource *source, GSourceFunc, gpointer)
{
    Source *src = static_cast<Source*>(source);
    if (!src)
    {
        return G_SOURCE_REMOVE;
    }
    auto rlSource = src->self.lock();
    if (!rlSource)
    {
        CWARN(event, "Orphan socket notifier source invoked!");
        return G_SOURCE_REMOVE;
    }

    auto callback = [](GPollHandler& poll)
    {
        if ((poll.fd.revents & poll.fd.events) != 0 && poll.notifier)
        {
            SocketNotifierSource::Notifier::Modes event = SocketNotifierSource::Notifier::Modes::Inactiv;
            SocketNotifierSource::Notifier::Modes reqEvents = poll.notifier->getModes();
            if ((poll.fd.revents & readMask) && (pollRead(reqEvents)))
            {
                event = SocketNotifierSource::Notifier::Modes::Read;
            }
            if ((poll.fd.revents & writeMask) && (pollWrite(reqEvents)))
            {
                event |= SocketNotifierSource::Notifier::Modes::Write;
            }
            if ((poll.fd.revents & exceptionMask) && (pollException(reqEvents)))
            {
                event |= SocketNotifierSource::Notifier::Modes::Exception;
            }
            if ((poll.fd.revents & errorMask) && (pollError(reqEvents)))
            {
                event |= SocketNotifierSource::Notifier::Modes::Error;
            }
            if (event != SocketNotifierSource::Notifier::Modes::Inactiv)
            {
                poll.notifier->signal(event);
            }
        }
    };
    for_each(rlSource->pollHandlers, callback);

    return G_SOURCE_CONTINUE;
}

/******************************************************************************
 * GSocketNotifierSource
 */
GSocketNotifierSource::GSocketNotifierSource(std::string_view name)
    : SocketNotifierSource(name)
{
}

GSocketNotifierSource::~GSocketNotifierSource()
{
    GSource* gsource = static_cast<GSource*>(source);
    auto cleanup = [gsource](GPollHandler& handler)
    {
        g_source_remove_poll(gsource, &handler.fd);
        handler.reset();
    };
    for_each(pollHandlers, cleanup);

    Source::destroy(source);
    CTRACE(event, "socket runloop source deleted");
}

void GSocketNotifierSource::initialize(void* data)
{
    CTRACE(event, "initialize SocketNotifier runloop source");
    source = Source::create(*this, reinterpret_cast<GMainContext*>(data));
}

void GSocketNotifierSource::detachOverride()
{
    CTRACE(event, "detach SocketNotifier runloop source");
    auto close = [](const GPollHandler& handler)
    {
        if (handler.notifier)
        {
            handler.notifier->detach();
        }
    };
    for_each(pollHandlers, close);
}

void GSocketNotifierSource::addNotifier(Notifier& notifier)
{
    pollHandlers.emplace_back(GPollHandler(notifier));
    g_source_add_poll(static_cast<GSource*>(source), &pollHandlers.back().fd);
}

void GSocketNotifierSource::removeNotifier(Notifier& notifier)
{
    auto predicate = [&notifier, this](GPollHandler& poll)
    {
        if (poll.notifier.get() == &notifier)
        {
            g_source_remove_poll(static_cast<GSource*>(this->source), &poll.fd);
            return true;
        }
        return false;
    };
    erase_if(pollHandlers, predicate);
}

/******************************************************************************
 * Factory
 */

SocketNotifierSourcePtr Adaptation::createSocketNotifierSource(std::string_view name)
{
    return SocketNotifierSourcePtr(new GSocketNotifierSource(name));
}

}
