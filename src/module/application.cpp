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

#ifndef APPLICATION_CPP
#define APPLICATION_CPP

#include <mox/event_handling/event_dispatcher.hpp>
#include <mox/event_handling/event_loop.hpp>
#include <mox/module/application.hpp>
#include <mox/module/thread_loop.hpp>

namespace mox
{

class ApplicationThread : public ThreadLoop
{
public:
    explicit ApplicationThread(Application& app)
        : app(app)
    {
    }
    ~ApplicationThread()
    {
        // Force stopped status to avoid joining.
        m_status.store(Status::Stopped);
    }

    void start() final
    {
    }

    int run() final
    {
        app.started();
        ThreadLoop::run();
        app.stopped();
        return threadData()->exitCode();
    }

    Application& app;
};

Application::Application(int argc, const char** argv)
    : m_mainThread(ThreadData::create())
    , m_rootObject(Object::create())
{
    m_mainThread->m_thread = make_polymorphic_shared<ThreadLoop, ApplicationThread>(*this);
    UNUSED(argc);
    UNUSED(argv);
}

Application::~Application()
{
    m_mainThread->m_thread.reset();
}

Application& Application::instance()
{
    return std::static_pointer_cast<ApplicationThread>(ThreadData::mainThread()->thread())->app;
}

ObjectSharedPtr Application::getRootObject() const
{
    return m_rootObject;
}

void Application::setRootObject(Object &root)
{
    lock_guard lock(*this);
    if (m_mainThread->thread()->isRunning())
    {
        return;
    }
    m_rootObject.reset();
    m_rootObject = root.shared_from_this();
}

ThreadDataSharedPtr Application::threadData() const
{
    return m_mainThread;
}

int Application::run()
{
    auto wipeRoot = [this]()
    {
        m_rootObject.reset();
    };
    stopped.connect(wipeRoot);

    return std::static_pointer_cast<ApplicationThread>(m_mainThread->thread())->run();
}

void Application::exit(int exitCode)
{
    m_mainThread->thread()->exit(exitCode);
}

void Application::quit()
{
    exit(0);
}

void Application::addIdleTask(EventDispatcher::IdleFunction&& task)
{
    m_mainThread->eventDispatcher()->addIdleTask(std::forward<decltype(task)>(task));
}

}

#endif // APPLICATION_CPP
