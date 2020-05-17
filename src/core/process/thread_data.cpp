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

#include <mox/core/process/thread_data.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/process/thread_loop.hpp>
#include <process_p.hpp>

namespace mox
{

/******************************************************************************
 * namespace td
 */
// One for the application.
static ThreadData* mainThreadData = nullptr;
// One for each thread.
static thread_local ThreadData* ltsThreadData = nullptr;

namespace td
{

void attachToThread(ThreadData& td)
{
    throwIf<ExceptionType::InvalidThreadData>(ltsThreadData != nullptr);
    if (!mainThreadData)
    {
        mainThreadData = &td;
    }
    ltsThreadData = &td;
}

void detachFromThread()
{
    if (ltsThreadData == mainThreadData)
    {
        mainThreadData = nullptr;
    }
    ltsThreadData = nullptr;
}

}
/******************************************************************************
 *
 */
ThreadData::ThreadData(ThreadInterface& td)
    : m_thread(as_shared<ThreadInterface>(td.shared_from_this()))
{
}

ThreadData::~ThreadData()
{
}

ThreadDataSharedPtr ThreadData::create(ThreadInterface& thread)
{
    ThreadDataSharedPtr threadData(new ThreadData(thread));
    return threadData;
}

ThreadDataSharedPtr ThreadData::getThisThreadData()
{
    return ltsThreadData
            ? ltsThreadData->shared_from_this()
            : nullptr;
}

ThreadDataSharedPtr ThreadData::getMainThreadData()
{
    return mainThreadData ? mainThreadData->shared_from_this() : nullptr;
}

bool ThreadData::isMainThread() const
{
    return mainThreadData == this;
}

ThreadInterfacePtr ThreadData::thread() const
{
    return m_thread;
}

}
