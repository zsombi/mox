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

/******************************************************************************
 *
 */
SocketNotifier::Modes SocketNotifier::supportedModes()
{
    return Modes::Read | Modes::Write | Modes::Error;
}

/******************************************************************************
 * CFSocketNotifiers::Socket
 */
CFSocketNotifierSource::Socket::Socket(CFSocketNotifierSource& socketSource, SocketNotifier& notifier)
    : socketSource(socketSource)
    , cfSource(nullptr)
    , notifiers([](const SocketNotifierSharedPtr& notifier) { return notifier == nullptr; })
    , handler(notifier.handler())
{
    // Create a CFSocket with both read and write, no matter if only one type is needed.
    constexpr int callbackTypes = kCFSocketReadCallBack | kCFSocketWriteCallBack;
    CFSocketContext context = {0, this, nullptr, nullptr, nullptr};
    cfSocket = CFSocketCreateWithNative(kCFAllocatorDefault, handler, callbackTypes, &Socket::callback, &context);
    FATAL(CFSocketIsValid(cfSocket), "Native socket creation failed")

    CFOptionFlags flags = CFSocketGetSocketFlags(cfSocket);
    // SocketNotifier doesn't close the socket on descruction.
    flags &= ~kCFSocketCloseOnInvalidate;

    addNotifier(notifier);
}

CFSocketNotifierSource::Socket::~Socket()
{
    if (CFSocketIsValid(cfSocket))
    {
        CFEventDispatcher* loop = static_cast<CFEventDispatcher*>(socketSource.eventDispatcher().get());
        CFRunLoopRemoveSource(loop->runLoop, cfSource, kCFRunLoopCommonModes);
        CFSocketDisableCallBacks(cfSocket, kCFSocketReadCallBack);
        CFSocketDisableCallBacks(cfSocket, kCFSocketWriteCallBack);
    }
    CFRunLoopSourceInvalidate(cfSource);
    CFRelease(cfSource);

    // No invalidation is needed, as if it was valid, it was removed from the runloop.
    CFRelease(cfSocket);
}

void CFSocketNotifierSource::Socket::addNotifier(SocketNotifier &notifier)
{
    notifiers.push_back(notifier.shared_from_this());
    if (notifier.hasReadMode())
    {
        if (!readNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketReadCallBack);
        }
        ++readNotifierCount;
    }
    if (notifier.hasWriteMode())
    {
        if (!writeNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketWriteCallBack);
        }
        ++writeNotifierCount;
    }
}

bool CFSocketNotifierSource::Socket::removeNotifier(SocketNotifier& notifier)
{
    auto notifierLookup = [&notifier](SocketNotifierSharedPtr n)
    {
        return n.get() == &notifier;
    };
    auto index = notifiers.find(notifierLookup);
    if (!index)
    {
        return false;
    }

    lock_guard lock(notifiers);
    notifiers[*index].reset();

    bool isRead = notifier.hasReadMode();
    bool isWrite = notifier.hasWriteMode();

    if (isRead && (readNotifierCount > 0))
    {
        --readNotifierCount;
        if (readNotifierCount <= 0)
        {
            CFSocketDisableCallBacks(cfSocket, kCFSocketReadCallBack);
        }
    }

    if (isWrite && (writeNotifierCount > 0))
    {
        --writeNotifierCount;
        if (writeNotifierCount <= 0)
        {
            CFSocketDisableCallBacks(cfSocket, kCFSocketWriteCallBack);
        }
    }

    return (!readNotifierCount && !writeNotifierCount);
}


void CFSocketNotifierSource::Socket::callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef, const void *, void *info)
{
    TRACE("Socket notified, dispatch")
    Socket* socket = static_cast<Socket*>(info);
    int nativeHandler = CFSocketGetNative(s);

    if (!socket || (socket->handler != nativeHandler))
    {
        return;
    }

    auto process = [&callbackType](SocketNotifierSharedPtr notifier)
    {
        if (!notifier)
        {
            return;
        }
        switch (callbackType)
        {
            case kCFSocketReadCallBack:
            {
                if (notifier->hasReadMode())
                {
                    notifier->activated(notifier, SocketNotifier::Modes::Read);
                }
                break;
            }
            case kCFSocketWriteCallBack:
            {
                if (notifier->hasWriteMode())
                {
                    notifier->activated(notifier, SocketNotifier::Modes::Write);
                }
                break;
            }
        }
    };
    socket->notifiers.forEach(process);
    TRACE("Leaving socket notifications.")
}

/******************************************************************************
 *
 */
CFSocketNotifierSource::CFSocketNotifierSource(std::string_view name)
    : SocketNotifierSource(name)
//    , sockets([](const SocketPtr& socket) { return !socket || !socket->handler; })
{
}

CFSocketNotifierSource::~CFSocketNotifierSource()
{
}

void CFSocketNotifierSource::enableSockets()
{
    for (auto& socket : sockets)
    {
        if (!socket || !CFSocketIsValid(socket->cfSocket))
        {
            continue;
        }

        if (!socket->cfSource)
        {
            socket->cfSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, socket->cfSocket, 0);
            if (socket->cfSource)
            {
                CFEventDispatcher* loop = static_cast<CFEventDispatcher*>(m_eventDispatcher.lock().get());
                FATAL(loop, "The event loop is destroyed!")
                CFRunLoopAddSource(loop->runLoop, socket->cfSource, loop->currentMode);
            }
            else
            {
                TRACE("CFSocketNotifier invalidated")
                CFSocketInvalidate(socket->cfSocket);
            }
        }
    }
}

void CFSocketNotifierSource::addNotifier(SocketNotifier& notifier)
{
    if ((notifier.modes() & SocketNotifier::Modes::Exception) == SocketNotifier::Modes::Exception)
    {
        return;
    }

    auto lookup = [fd = notifier.handler()](const SocketPtr& socket)
    {
        return socket->handler == fd;
    };
    auto socket = std::find_if(sockets.begin(), sockets.end(), lookup);
    if (socket == sockets.end())
    {
        sockets.emplace_back(std::make_unique<Socket>(*this, notifier));
        TRACE("Socket:: " << sockets.back().get())
    }
    else
    {
        // Add this notifier
        (*socket)->addNotifier(notifier);
    }
}

void CFSocketNotifierSource::removeNotifier(SocketNotifier& notifier)
{
    auto lookup = [fd = notifier.handler()](const SocketPtr& socket)
    {
        return socket->handler == fd;
    };
    auto socket = std::find_if(sockets.begin(), sockets.end(), lookup);
    if (socket == sockets.end())
    {
        return;
    }

    if ((*socket)->removeNotifier(notifier))
    {
        // remove the socket from the pool
        sockets.erase(socket);
    }
}

void CFSocketNotifierSource::prepare()
{
}

void CFSocketNotifierSource::shutDown()
{
    TRACE("Shutting down sockets")
    sockets.clear();
    TRACE("SocketNotifiers down");
}

/******************************************************************************
 *
 */
SocketNotifierSourcePtr Adaptation::createSocketNotifierSource(std::string_view name)
{
    return make_polymorphic_shared<SocketNotifierSource, CFSocketNotifierSource>(name);
}

}
