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

#include <mox/core/event_handling/run_loop.hpp>
#include <mox/core/process/application.hpp>
#include <process_p.hpp>

namespace mox
{

class ApplicationThread : public ThreadInterface
{
public:
    explicit ApplicationThread(Application& app)
        : app(app)
    {
    }
    ~ApplicationThread()
    {
    }

    static auto create(Application& app)
    {
        return make_thread(new ApplicationThread(app));
    }

    int run()
    {
        start();

        app.started();
        auto d = ThreadInterfacePrivate::get(*this);
        d->statusProperty = Status::Running;

        auto runLoop = as_shared<RunLoop>(d->runLoop);
        runLoop->execute();

        d->statusProperty = Status::Stopped;
        app.stopped();

        tearDown();
        return d->exitCodeProperty;
    }

    Application& app;

protected:

    RunLoopBasePtr createRunLoopOverride() final
    {
        return RunLoop::create(true);
    }

    void initialize() override
    {
        ThreadInterface::initialize();

        auto d = ThreadInterfacePrivate::get(*this);
        ScopeValue tmp(d->statusProperty, Status::StartingUp);
        setUp();
    }

    void startOverride() final
    {
    }
};

Application::Application(int argc, const char** argv)
{
    auto thread = ApplicationThread::create(*this);
    m_threadData = thread->threadData();
    throwIf<ExceptionType::InvalidThreadData>(!m_threadData);

    m_rootObject = Object::create();
    UNUSED(argc);
    UNUSED(argv);
}

Application::~Application()
{
    m_threadData.reset();
}

Application& Application::instance()
{
    return as_shared<ApplicationThread>(ThreadData::getMainThreadData()->thread())->app;
}

ObjectSharedPtr Application::getRootObject() const
{
    return m_rootObject;
}

void Application::setRootObject(Object &root)
{
    lock_guard lock(*this);
    m_rootObject.reset();
    m_rootObject = as_shared<Object>(&root);
}

int Application::run()
{
    return as_shared<ApplicationThread>(m_threadData->thread())->run();
}

void Application::exit(int exitCode)
{
    m_threadData->thread()->exit(exitCode);
}

void Application::quit()
{
    exit();
}

}

#endif // APPLICATION_CPP
