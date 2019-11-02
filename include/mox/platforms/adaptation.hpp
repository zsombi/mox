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

#ifndef ADAPTATION_HPP

#define ADAPTATION_HPP

#include <string_view>
#include <mox/event_handling/event_handling_declarations.hpp>

namespace mox
{

class MOX_API Adaptation
{
    explicit Adaptation() = default;
public:
    static EventDispatcherSharedPtr createEventDispatcher(ThreadData& threadData, bool main);
    static TimerSourcePtr createTimerSource(std::string_view name);
    static PostEventSourcePtr createPostEventSource(std::string_view name);
    static SocketNotifierSourcePtr createSocketNotifierSource(std::string_view name);
};

} // namespace mox

#endif // ADAPTATION_HPP