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

void CFIdleSource::addIdleTaskOverride(Task&& task)
{
    tasks.push(std::forward<Task>(task));
}

void CFIdleSource::initialize(void*)
{
}

void CFIdleSource::detachOverride()
{
}

void CFIdleSource::wakeUp()
{
}

size_t CFIdleSource::runTasks()
{
    auto taskCopy = TaskStack();
    std::swap(taskCopy, tasks);
    while (!taskCopy.empty())
    {
        auto task = std::move(taskCopy.top());
        taskCopy.pop();

        if (!task())
        {
            tasks.push(task);
        }
    }
    return tasks.size();
}

/******************************************************************************
 * Adaptation
 */
IdleSourcePtr Adaptation::createIdleSource()
{
    return make_polymorphic_shared<IdleSource, CFIdleSource>();
}

} // mox
