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
static void processEvents(void *info)
{
    CFPostEventSource* source = static_cast<CFPostEventSource*>(info);
    source->dispatch();
}

CFPostEventSource::CFPostEventSource(std::string_view name)
    : PostEventSource(name)
{
}

CFPostEventSource::~CFPostEventSource()
{
    CFRunLoopSourceInvalidate(sourceRef);
    CFRelease(sourceRef);
}

void CFPostEventSource::setEventDispatcher(EventDispatcher &eventDispatcher)
{
    PostEventSource::setEventDispatcher(eventDispatcher);

    CFRunLoopSourceContext context = {};
    context.info = this;
    context.perform = processEvents;

    sourceRef = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &context);
    CFEventDispatcher& loop = static_cast<CFEventDispatcher&>(eventDispatcher);
    CFRunLoopAddSource(loop.runLoop, sourceRef, kCFRunLoopCommonModes);
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
PostEventSourcePtr Adaptation::createPostEventSource(std::string_view name)
{
    return PostEventSourcePtr(new CFPostEventSource(name));
}

}
