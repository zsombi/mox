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
#include <mox/utils/globals.hpp>

namespace mox
{

SocketNotifier::Modes SocketNotifier::supportedModes()
{
    return Modes::Read | Modes::Write | Modes::Error | Modes::Exception;
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

constexpr bool pollRead(SocketNotifier::Modes modes)
{
    return (modes & SocketNotifier::Modes::Read) == SocketNotifier::Modes::Read;
}
constexpr bool pollWrite(SocketNotifier::Modes modes)
{
    return (modes & SocketNotifier::Modes::Write) == SocketNotifier::Modes::Write;
}
constexpr bool pollException(SocketNotifier::Modes modes)
{
    return (modes & SocketNotifier::Modes::Exception) == SocketNotifier::Modes::Exception;
}
constexpr bool pollError(SocketNotifier::Modes modes)
{
    return (modes & SocketNotifier::Modes::Error) == SocketNotifier::Modes::Error;
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

GPollHandler::GPollHandler(SocketNotifier& notifier)
{
    this->notifier = notifier.shared_from_this();
    fd.fd = notifier.handler();
    fd.events = 0;

    SocketNotifier::Modes modes = notifier.modes();
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

GSocketNotifierSource::Source* GSocketNotifierSource::Source::create(GSocketNotifierSource& socketSource)
{
    Source *src = reinterpret_cast<Source*>(g_source_new(&socketNotifierSourceFuncs, sizeof(*src)));

    src->self = &socketSource;

    GSource* source = static_cast<GSource*>(src);
//    g_source_set_can_recurse(source, true);
    GlibEventDispatcher* loop = static_cast<GlibEventDispatcher*>(socketSource.eventDispatcher().get());
    g_source_attach(source, loop->context);

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

    //check for pending and remove orphaned entries
    bool hasPendingEvents = false;

    auto callback = [&hasPendingEvents](GPollHandler& poll)
    {
        if (poll.fd.revents & G_IO_NVAL)
        {
            // Disable the invalid socket notifier. This will at least reset the poll.
            poll.notifier->setEnabled(false);
        }
        else
        {
            hasPendingEvents = hasPendingEvents || ((poll.fd.revents & poll.fd.events) != 0);
        }
    };
    lock_guard lock(src->self->pollHandlers);
    src->self->pollHandlers.forEach(callback);

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

    auto callback = [](GPollHandler& poll)
    {
        if ((poll.fd.revents & poll.fd.events) != 0 && poll.notifier)
        {
            SocketNotifier::Modes event = SocketNotifier::Modes::Inactiv;
            SocketNotifier::Modes reqEvents = poll.notifier->modes();
            if ((poll.fd.revents & readMask) && (pollRead(reqEvents)))
            {
                event = SocketNotifier::Modes::Read;
            }
            if ((poll.fd.revents & writeMask) && (pollWrite(reqEvents)))
            {
                event |= SocketNotifier::Modes::Write;
            }
            if ((poll.fd.revents & exceptionMask) && (pollException(reqEvents)))
            {
                event |= SocketNotifier::Modes::Exception;
            }
            if ((poll.fd.revents & errorMask) && (pollError(reqEvents)))
            {
                event |= SocketNotifier::Modes::Error;
            }
            if (event != SocketNotifier::Modes::Inactiv)
            {
                poll.notifier->activated(poll.notifier, event);
            }
        }
    };
    lock_guard lock(src->self->pollHandlers);
    src->self->pollHandlers.forEach(callback);

    return G_SOURCE_CONTINUE;
}

/******************************************************************************
 * GSocketNotifierSource
 */
GSocketNotifierSource::GSocketNotifierSource(std::string_view name)
    : SocketNotifierSource(name)
    , pollHandlers([](const GPollHandler& handler) { return handler.notifier == nullptr; })
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
    pollHandlers.forEach(cleanup);

    Source::destroy(source);
}

void GSocketNotifierSource::prepare()
{
    source = Source::create(*this);
}

void GSocketNotifierSource::shutDown()
{
    auto close = [](const GPollHandler& handler)
    {
        if (handler.notifier)
        {
            handler.notifier->setEnabled(false);
        }
    };
    pollHandlers.forEach(close);
}

void GSocketNotifierSource::addNotifier(SocketNotifier& notifier)
{
    pollHandlers.emplace_back(GPollHandler(notifier));
    g_source_add_poll(static_cast<GSource*>(source), &pollHandlers.back().fd);
}

void GSocketNotifierSource::removeNotifier(SocketNotifier& notifier)
{
    auto predicate = [&notifier](const GPollHandler& poll)
    {
        return poll.notifier.get() == &notifier;
    };
    auto index = pollHandlers.find(predicate);
    if (!index)
    {
        return;
    }

    // Lock the container to make sure the container size is not altered.
    lock_guard lock(pollHandlers);
    g_source_remove_poll(static_cast<GSource*>(source), &pollHandlers[*index].fd);
    pollHandlers[*index].reset();
}

/******************************************************************************
 * Factory
 */

SocketNotifierSourcePtr Adaptation::createSocketNotifierSource(std::string_view name)
{
    return SocketNotifierSourcePtr(new GSocketNotifierSource(name));
}

}
