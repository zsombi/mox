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

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <mox/mox_module.hpp>
#include <mox/core/object.hpp>
#include <mox/core/event_handling/run_loop.hpp>

namespace mox
{

/// The Application class provides support for a main loop in your application. You can have only
/// one instance of this class in your application.
class MOX_API Application : public Object
{
public:
    /// The static metaclass of the Application class.
    /// Signal is emitted when the application's event loop is started.
    metakernel::Signal<> started{*this};
    /// Signal emitted when the application's event loop exits.
    metakernel::Signal<> stopped{*this};

    /// Constructor, creates an application object. You can have only one application object in your
    /// application.
    explicit Application(int argc = 0, const char** argv = nullptr);

    /// Destructor.
    ~Application() override;

    /// Returns the instance of the Application.
    static Application& instance();

    /// Returns the root object of the application.
    /// \return The root object of the application.
    ObjectSharedPtr getRootObject() const;

    /// Sets the root object of the application. The application must have a root object.
    /// The previous root object is deleted together with its child objects. To avoid this, you
    /// must move all child objects to the new root before replacing the root object.
    /// \param root The root object of the application.
    void setRootObject(Object& root);

    /// Returns the root object dynamic-casted to a given TargetType.
    template <class TargetType>
    std::shared_ptr<TargetType> castRootObject() const
    {
        return std::dynamic_pointer_cast<TargetType>(getRootObject());
    }

    /// Executes the application's main event loop.
    /// \return The exit code.
    int run();

    /// Exit the running application. Optionally, you can pass an exit code.
    /// \param exitCode The exit code to use when exiting the application's main loop.
    void exit(int exitCode = 0);

    /// Quits the application.
    void quit();

private:
    ObjectSharedPtr m_rootObject;
};

}

#endif // APPLICATION_HPP
