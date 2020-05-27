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

#include <mox/mox_module.hpp>
#include <mox/core/event_handling/socket_notifier.hpp>
#include <mox/core/object.hpp>
#include <mox/core/timer.hpp>
#include <mox/core/process/application.hpp>
#include <mox/core/process/thread_loop.hpp>
#include <mox/core/process/applet.hpp>

#include <vector>

namespace mox
{

void MoxModule::registerModule()
{
    registerMetaType<SocketNotifierSharedPtr>("shared_ptr<SocketNotifier>");
//    registerMetaClass<SocketNotifier>();
//    registerMetaClass<Timer>();
//    registerMetaClass<MetaObject>();
//    registerMetaClass<Object>();
//    registerMetaClass<ThreadInterface>();
//    registerMetaClass<ThreadLoop>();
//    registerMetaClass<Application>();
//    registerMetaClass<Applet>();
}

}
