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
#include <mox/metadata/metaobject.hpp>
#include <mox/signal/signal.hpp>
#include <mox/event_handling/socket_notifier.hpp>
#include <mox/object.hpp>
#include <mox/module/application.hpp>
#include <mox/module/thread_loop.hpp>

#include <vector>

namespace mox
{

void MoxModule::registerModule()
{
    registerMetaType<SocketNotifierSharedPtr>();
    registerMetaClass<MetaObject>("MetaObject");
    registerMetaClass<Object>("Object");
    registerMetaClass<ThreadLoop>("ThreadLoop");
    registerMetaClass<Application>("Application");
}

}
