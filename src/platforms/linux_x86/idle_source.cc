/*
 * Copyright (C) 2017-2020 bitWelder
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

GIdleSource::TaskRec::~TaskRec()
{
    if (source)
    {
        g_source_destroy(source);
    }
}

GIdleSource::GIdleSource()
{
}

GIdleSource::~GIdleSource()
{
    CTRACE(event, "idle runloop source destroyed" << (void*)this);
}

gboolean GIdleSource::sourceFunc(gpointer userData)
{
    TaskRec* record = static_cast<TaskRec*>(userData);
    auto idle = record->self.lock();
    if (!idle)
    {
        CWARN(event, "Orphan idle source invoked!");
        return false;
    }

    CTRACE(event, "Idle source activated" << idle.get());

    auto runLoop = idle->getRunLoop();
    if (!runLoop || !runLoop->isRunning() || runLoop->isExiting())
    {
        // returning false auto-destroyes the source
        return false;
    }

    // call the task
    auto result = !record->task();
    if (runLoop->isExiting())
    {
        // No matter of the result, return false to destroy idle source.
        result = false;
    }

    if (result)
    {
        // re-schedule runloop
        idle->getRunLoop()->scheduleSources();
    }
    return result;
}

void GIdleSource::sourceDestroy(gpointer userData)
{
    UNUSED(userData);
    TaskRec* record = static_cast<TaskRec*>(userData);
    auto idle = record->self.lock();
    if (!idle)
    {
        CWARN(event, "Orphan idle source invoked!");
        return;
    }
    CTRACE(event, "Idle source destroyed:" << idle.get() << "remove idle task record");

//    record->source = nullptr;
    idle->removeTaskRec(*record);
}

void GIdleSource::initialize(void* data)
{
    CTRACE(event, "initialize Idle runloop source");
    context = reinterpret_cast<GMainContext*>(data);
}

void GIdleSource::detachOverride()
{
    CTRACE(event, "detach Idle runloop source");
}

void GIdleSource::addIdleTaskOverride(Task&& task)
{
    if (!isFunctional())
    {
        return;
    }

    CTRACE(event, "create Idle source for" << (void*)this);
    tasks.emplace_back(std::make_unique<TaskRec>(as_shared<GIdleSource>(shared_from_this()), g_idle_source_new(), std::move(task)));
    auto& record = tasks.back();

    g_source_set_callback(record->source, &GIdleSource::sourceFunc, record.get(), &GIdleSource::sourceDestroy);
    g_source_attach(record->source, context);
    g_source_unref(record->source);
}

void GIdleSource::wakeUp()
{
    if (!isFunctional())
    {
        return;
    }
    CTRACE(event, "wake up Idle source for" << (void*)this);
}

void GIdleSource::removeTaskRec(TaskRec& record)
{
    auto removeRecord = [&record](auto& trec)
    {
        return trec.get() == &record;
    };

    erase_if(tasks, removeRecord);
}

/******************************************************************************
 * Adaptation
 */
IdleSourcePtr Adaptation::createIdleSource()
{
    return make_polymorphic_shared<IdleSource, GIdleSource>();
}

} // mox
