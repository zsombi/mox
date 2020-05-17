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

namespace
{

bool hasMode(const SocketNotifierSource::Notifier& notifier, SocketNotifierSource::Notifier::Modes mode)
{
    return (notifier.getModes() & mode) == mode;
}

}

/******************************************************************************
 *
 */
SocketNotifierSource::Notifier::Modes SocketNotifierSource::supportedModes()
{
    return SocketNotifierSource::Notifier::Modes::Read | SocketNotifierSource::Notifier::Modes::Write | SocketNotifierSource::Notifier::Modes::Error;
}

/******************************************************************************
 * CFSocketNotifiers::Socket
 */
CFSocketNotifierSource::Socket::Socket(CFSocketNotifierSource& socketSource, Notifier& notifier)
    : socketSource(socketSource)
    , cfSource(nullptr)
    , handler(notifier.handler())
{
    // Create a CFSocket with both read and write, no matter if only one type is needed.
    constexpr int callbackTypes = kCFSocketReadCallBack | kCFSocketWriteCallBack;
    CFSocketContext context = {0, this, nullptr, nullptr, nullptr};
    cfSocket = CFSocketCreateWithNative(kCFAllocatorDefault, handler, callbackTypes, &Socket::callback, &context);
    FATAL(CFSocketIsValid(cfSocket), "Native socket creation failed");

    CFOptionFlags flags = CFSocketGetSocketFlags(cfSocket);
    // SocketNotifier doesn't close the socket on descruction.
    flags &= ~kCFSocketCloseOnInvalidate;

    addNotifier(notifier);
}

CFSocketNotifierSource::Socket::~Socket()
{
    if (CFSocketIsValid(cfSocket))
    {
        auto loop = std::dynamic_pointer_cast<FoundationConcept>(socketSource.getRunLoop());
        loop->removeSource(cfSource);
        CFSocketDisableCallBacks(cfSocket, kCFSocketReadCallBack);
        CFSocketDisableCallBacks(cfSocket, kCFSocketWriteCallBack);
    }
    CFRunLoopSourceInvalidate(cfSource);
    CFRelease(cfSource);

    // No invalidation is needed, as if it was valid, it was removed from the runloop.
    CFRelease(cfSocket);
}

void CFSocketNotifierSource::Socket::addNotifier(Notifier& notifier)
{
    notifiers.push_back(notifier.shared_from_this());
    if (hasMode(notifier, SocketNotifierSource::Notifier::Modes::Read))
    {
        if (!readNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketReadCallBack);
        }
        ++readNotifierCount;
    }
    if (hasMode(notifier, SocketNotifierSource::Notifier::Modes::Write))
    {
        if (!writeNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketWriteCallBack);
        }
        ++writeNotifierCount;
    }
}

bool CFSocketNotifierSource::Socket::removeNotifier(Notifier& notifier)
{
    auto remover = [&notifier](SocketNotifierSource::NotifierPtr n)
    {
        return n.get() == &notifier;
    };
    if (!erase_if(notifiers, remover))
    {
        return false;
    }

    bool isRead = hasMode(notifier, SocketNotifierSource::Notifier::Modes::Read);
    bool isWrite = hasMode(notifier, SocketNotifierSource::Notifier::Modes::Write);

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
    CTRACE(event, "Socket notified, dispatch");
    Socket* socket = static_cast<Socket*>(info);
    int nativeHandler = CFSocketGetNative(s);

    if (!socket || (socket->handler != nativeHandler))
    {
        return;
    }

    auto process = [&callbackType](SocketNotifierSource::NotifierPtr notifier)
    {
        if (!notifier)
        {
            return;
        }
        switch (callbackType)
        {
            case kCFSocketReadCallBack:
            {
                if (hasMode(*notifier, SocketNotifierSource::Notifier::Modes::Read))
                {
                    notifier->signal(SocketNotifierSource::Notifier::Modes::Read);
                }
                break;
            }
            case kCFSocketWriteCallBack:
            {
                if (hasMode(*notifier, SocketNotifierSource::Notifier::Modes::Write))
                {
                    notifier->signal(SocketNotifierSource::Notifier::Modes::Write);
                }
                break;
            }
        }
    };
    for_each(socket->notifiers, process);
    CTRACE(event, "Leaving socket notifications.");
}

/******************************************************************************
 *
 */
CFSocketNotifierSource::CFSocketNotifierSource(std::string_view name)
    : SocketNotifierSource(name)
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
                auto loop = std::dynamic_pointer_cast<FoundationConcept>(m_runLoop.lock());
                FATAL(loop, "The event loop is destroyed!");
                loop->addSource(socket->cfSource);
            }
            else
            {
                CTRACE(event, "CFSocketNotifier invalidated");
                CFSocketInvalidate(socket->cfSocket);
            }
        }
    }
}

void CFSocketNotifierSource::addNotifier(Notifier& notifier)
{
    auto lookup = [fd = notifier.handler()](const SocketPtr& socket)
    {
        return socket->handler == fd;
    };
    auto socket = std::find_if(sockets.begin(), sockets.end(), lookup);
    if (socket == sockets.end())
    {
        sockets.emplace_back(std::make_unique<Socket>(*this, notifier));
        CTRACE(event, "Socket::" << sockets.back().get());
    }
    else
    {
        // Add this notifier
        (*socket)->addNotifier(notifier);
    }
}

void CFSocketNotifierSource::removeNotifier(Notifier& notifier)
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

void CFSocketNotifierSource::detachOverride()
{
    CTRACE(event, "Shutting down sockets");
    sockets.clear();
    CTRACE(event, "SocketNotifiers down");
}

/******************************************************************************
 *
 */
SocketNotifierSourcePtr Adaptation::createSocketNotifierSource(std::string_view name)
{
    return make_polymorphic_shared<SocketNotifierSource, CFSocketNotifierSource>(name);
}

}
