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

#ifndef THREAD_DATA_HPP
#define THREAD_DATA_HPP

#include <mox/config/platform_config.hpp>
#include <memory>

namespace mox
{

class ThreadData;
using ThreadDataSharedPtr = std::shared_ptr<ThreadData>;
using ThreadDataWeakPtr = std::weak_ptr<ThreadData>;

class ThreadInterface;
using ThreadInterfacePtr = std::shared_ptr<ThreadInterface>;
using ThreadInterfaceWeakPtr = std::weak_ptr<ThreadInterface>;

/// ThreadData contains the thread loop of a thread in Mox. You must have a thread data on each
/// thread that handles events, or has signal-slots connected to other threads.
class MOX_API ThreadData : public std::enable_shared_from_this<ThreadData>
{
public:
    /// Destructor.
    virtual ~ThreadData();

    /// Creates a thread data. You should create the thread data for the runnig thread from within
    /// the thread. Calling the method on a thread with a valid thread data throws exception.
    /// \param thread The thread object to which the thread data is attached.
    /// \return The thread data for the thread.
    /// \throws ExceptionType::ThreadWithThreadData
    static ThreadDataSharedPtr create(ThreadInterface& thread);

    /// Returns the thread data of this thread.
    /// \return The thread data for this thread, or \e nullptr if the thread has no thread data yet.
    static ThreadDataSharedPtr getThisThreadData();

    /// Returns the main thread data.
    static ThreadDataSharedPtr getMainThreadData();

    /// Checks whether the thread data is the main thread's data.
    /// \return If the thread data belongs to the main thread, returns \e true, otherwise \e false.
    bool isMainThread() const;

    /// Returns the thread object owning the thread data.
    /// \return The thread object owning the thread data.
    ThreadInterfacePtr thread() const;

protected:
    /// Constructor.
    explicit ThreadData(ThreadInterface& td);

    ThreadInterfacePtr m_thread;
};

}

#endif // THREAD_DATA_HPP
