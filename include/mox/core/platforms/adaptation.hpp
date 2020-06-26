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
#include <mox/core/event_handling/event_handling_declarations.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox
{

class ThreadData;

class MOX_API Adaptation
{
    explicit Adaptation() = default;
public:
    static RunLoopPtr createRunLoop(bool main);
    static RunLoopHookPtr createRunLoopHook();
    static SocketNotifierCore::Modes supportedModes();
};

} // namespace mox

DECLARE_LOG_CATEGORY(platform)

#endif // ADAPTATION_HPP
