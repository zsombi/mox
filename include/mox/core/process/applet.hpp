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

#ifndef APPLET_HPP
#define APPLET_HPP

#include <mox/core/process/thread_interface.hpp>

namespace mox
{

class Applet;
using AppletPtr = std::shared_ptr<Applet>;

/// An applet enables you to integrate Mox functionality in a native application's event loop.
/// Once created, you must start the applet to attach to a running application.
/// You must stop the applet when your native application's event loop stops.
class MOX_API Applet : public ThreadInterface
{
public:
    /// Creates an applet.
    static AppletPtr create();

protected:
    /// Constructor.
    explicit Applet();

    /// Overrides from ThreadInterface.
    void initialize() override;
    RunLoopBasePtr createRunLoopOverride() override;
    void startOverride() override;
    void quitOverride() override;
};

}

#endif // APPLET_HPP
