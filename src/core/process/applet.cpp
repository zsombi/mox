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

#include <mox/core/process/applet.hpp>
#include <mox/core/event_handling/run_loop.hpp>
#include <process_p.hpp>

namespace mox
{

Applet::Applet()
{
}

AppletPtr Applet::create()
{
    return make_thread(new Applet);
}

void Applet::initialize()
{
    ThreadInterface::initialize();
    D();
    // Put the applet to StartingUp state temporarily so we can do the setup of the thread.
    ScopeValue tempStatus(d->statusProperty, Status::StartingUp);
    setUp();
}

RunLoopBasePtr Applet::createRunLoopOverride()
{
    auto runLoop = RunLoopHook::create();
    auto onHookClosed = [this, d = d_func()]()
    {
        CTRACE(threads, "hook closed");

        {
            ScopeRelock unlocker(*this);
            this->stopped(this);
            // Tear down applet.
            this->tearDown();
        }

        return true;
    };
    runLoop->setRunLoopDownCallback(onHookClosed);
    return runLoop;
}

void Applet::startOverride()
{
    started(this);
    auto d = ThreadInterfacePrivate::get(*this);
    d->statusProperty = Status::Running;
    CTRACE(threads, "boot up applet");
}

void Applet::joinOverride()
{
    throwIf<ExceptionType::JoiningApplet>(true);
}

void Applet::quitOverride()
{
    D();
    d->statusProperty = Status::Stopped;
    CTRACE(threads, "ramp down applet");
}

}
