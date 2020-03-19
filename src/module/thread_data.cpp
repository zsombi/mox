/*
 * Copyright (C) 2017-2018 bitWelder
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

#include <mox/module/thread_data.hpp>
#include <mox/event_handling/run_loop.hpp>
#include <mox/module/thread_loop.hpp>

namespace mox
{

// One for the application.
static ThreadData* mainThreadData = nullptr;
// One for each thread.
static thread_local ThreadData* ltsThreadData = nullptr;

/******************************************************************************
 *
 */
ThreadData::ThreadData()
{
    FATAL(!ltsThreadData, "This thread already has a thread data specified!");
    ltsThreadData = this;
    if (!mainThreadData)
    {
        mainThreadData = this;
    }
}

ThreadData::~ThreadData()
{
    ltsThreadData = nullptr;
    if (mainThreadData == this)
    {
        mainThreadData = nullptr;
    }
}

ThreadDataSharedPtr ThreadData::create()
{
    ThreadDataSharedPtr threadData(new ThreadData);
    return threadData;
}

ThreadDataSharedPtr ThreadData::thisThreadData()
{
    return ltsThreadData
            ? ltsThreadData->shared_from_this()
            : nullptr;
}

ThreadDataSharedPtr ThreadData::mainThread()
{
    return mainThreadData ? mainThreadData->shared_from_this() : nullptr;
}

bool ThreadData::isMainThread() const
{
    return mainThreadData == this;
}

ThreadLoopSharedPtr ThreadData::thread() const
{
    return m_thread;
}

}
