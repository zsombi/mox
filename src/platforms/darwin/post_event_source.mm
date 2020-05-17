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
static void execute(void *info)
{
    auto source = static_cast<CFPostEventSource*>(info);
    auto keepAlive = as_shared<EventSource>(source->shared_from_this());
    keepAlive->dispatchQueuedEvents();
}

CFPostEventSource::CFPostEventSource(std::string_view name)
    : EventSource(name)
{
}

CFPostEventSource::~CFPostEventSource()
{
}

void CFPostEventSource::initialize(void* data)
{
    auto runLoop = getRunLoop();
    FATAL(runLoop, "cannot prepare an invalid source");

    CFRunLoopSourceContext context = {};
    context.info = this;
    context.perform = execute;

    sourceRef = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    auto loopRef = static_cast<CFRunLoopRef>(data);
    CFRunLoopAddSource(loopRef, sourceRef, kCFRunLoopCommonModes);
}

void CFPostEventSource::detachOverride()
{
    CFRunLoopSourceInvalidate(sourceRef);
    CFRelease(sourceRef);
}

void CFPostEventSource::wakeUp()
{
    if (!sourceRef)
    {
        return;
    }
    CFRunLoopSourceSignal(sourceRef);
}

/******************************************************************************
 *
 */
EventSourcePtr Adaptation::createPostEventSource(std::string_view name)
{
    return make_polymorphic_shared<EventSource, CFPostEventSource>(name);
}

}
