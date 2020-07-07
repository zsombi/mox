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

bool hasMode(const SocketNotifierCore& notifier, SocketNotifierCore::Modes mode)
{
    return (notifier.getModes() & mode) == mode;
}

}

/******************************************************************************
 *
 */
SocketNotifierCore::Modes Adaptation::supportedModes()
{
    return SocketNotifierCore::Modes::Read
            | SocketNotifierCore::Modes::Write
            | SocketNotifierCore::Modes::Error;
}

/******************************************************************************
 * CFSocketNotifiers::Socket
 */
FoundationConcept::SocketRecord::SocketRecord(FoundationConcept& concept, SocketNotifierCore& notifier)
    : concept(concept)
    , cfSource(nullptr)
    , handler(notifier.handler())
{
    // Create a CFSocket with both read and write, no matter if only one type is needed.
    constexpr int callbackTypes = kCFSocketReadCallBack | kCFSocketWriteCallBack;
    CFSocketContext context = {0, this, nullptr, nullptr, nullptr};
    cfSocket = CFSocketCreateWithNative(kCFAllocatorDefault, handler, callbackTypes, &SocketRecord::callback, &context);
    FATAL(CFSocketIsValid(cfSocket), "Native socket creation failed");

    CFOptionFlags flags = CFSocketGetSocketFlags(cfSocket);
    // SocketNotifier doesn't close the socket on descruction.
    flags &= ~kCFSocketCloseOnInvalidate;

    addNotifier(notifier);
}

FoundationConcept::SocketRecord::~SocketRecord()
{
    if (CFSocketIsValid(cfSocket))
    {
        CFRunLoopRemoveSource(concept.runLoop, cfSource, concept.currentMode /*kCFRunLoopCommonModes*/);
        CFSocketDisableCallBacks(cfSocket, kCFSocketReadCallBack);
        CFSocketDisableCallBacks(cfSocket, kCFSocketWriteCallBack);
    }
    CFRunLoopSourceInvalidate(cfSource);
    CFRelease(cfSource);

    // No invalidation is needed, as if it was valid, it was removed from the runloop.
    CFRelease(cfSocket);
}

void FoundationConcept::SocketRecord::addNotifier(SocketNotifierCore& notifier)
{
    notifiers.push_back(notifier.shared_from_this());
    if (hasMode(notifier, SocketNotifierCore::Modes::Read))
    {
        if (!readNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketReadCallBack);
        }
        ++readNotifierCount;
    }
    if (hasMode(notifier, SocketNotifierCore::Modes::Write))
    {
        if (!writeNotifierCount)
        {
            // (re)enable callback.
            CFSocketEnableCallBacks(cfSocket, kCFSocketWriteCallBack);
        }
        ++writeNotifierCount;
    }
}

bool FoundationConcept::SocketRecord::removeNotifier(SocketNotifierCore& notifier)
{
    auto remover = [&notifier](SocketNotifierCorePtr n)
    {
        return n.get() == &notifier;
    };
    if (!erase_if(notifiers, remover))
    {
        return false;
    }

    bool isRead = hasMode(notifier, SocketNotifierCore::Modes::Read);
    bool isWrite = hasMode(notifier, SocketNotifierCore::Modes::Write);

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


void FoundationConcept::SocketRecord::callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef, const void *, void *info)
{
    CTRACE(event, "Socket notified, dispatch");
    auto socket = static_cast<SocketRecord*>(info);
    int nativeHandler = CFSocketGetNative(s);

    if (!socket || (socket->handler != nativeHandler))
    {
        return;
    }

    auto process = [&callbackType](SocketNotifierCorePtr notifier)
    {
        if (!notifier)
        {
            return;
        }
        switch (callbackType)
        {
            case kCFSocketReadCallBack:
            {
                if (hasMode(*notifier, SocketNotifierCore::Modes::Read))
                {
                    notifier->signal(SocketNotifierCore::Modes::Read);
                }
                break;
            }
            case kCFSocketWriteCallBack:
            {
                if (hasMode(*notifier, SocketNotifierCore::Modes::Write))
                {
                    notifier->signal(SocketNotifierCore::Modes::Write);
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

void FoundationConcept::enableSocketNotifiers()
{
    lock_guard lock(*this);
    auto enabler = [this](auto& socket)
    {
        if (!socket || !CFSocketIsValid(socket->cfSocket))
        {
            return;
        }

        if (!socket->cfSource)
        {
            socket->cfSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, socket->cfSocket, 0);
            if (socket->cfSource)
            {
                CFRunLoopAddSource(runLoop, socket->cfSource, currentMode);
            }
            else
            {
                CTRACE(event, "CFSocketNotifier invalidated");
                CFSocketInvalidate(socket->cfSocket);
            }
        }
    };
    for_each(sockets, enabler);
}

void FoundationConcept::clearSocketNotifiers()
{
    lock_guard lock(*this);
    CTRACE(event, "Shutting down sockets");
    sockets.clear();
    CTRACE(event, "SocketNotifiers down");
}

}
