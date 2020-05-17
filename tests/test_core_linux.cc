#include <mox/config/platform_config.hpp>
#include <test_framework.h>

#include <glib.h>


class TestCoreApp::Private
{
    static gboolean defaultIdleFunc(gpointer userData)
    {
        CTRACE(threads, "idle called for:" << userData);
        Private* app = static_cast<Private*>(userData);
        g_source_destroy(app->idleSource);
        app->idleSource = nullptr;
        return app->idle();
    }

public:
    GMainContext* context = nullptr;
    GMainLoop* runLoop = nullptr;
    GSource* idleSource = nullptr;
    mox::IdleSource::Task idle;

    Private()
    {
        CTRACE(threads, "app private created: " << (void*)this);
        context = g_main_context_default();
        FATAL(context, "The main context is not defined!");
        g_main_context_push_thread_default(context);
        g_main_context_ref(context);
        runLoop = g_main_loop_new(context, false);

    }
    ~Private()
    {
        if (idleSource)
        {
            g_source_unref(idleSource);
            idleSource = nullptr;
        }
        g_main_loop_unref(runLoop);
        g_main_context_pop_thread_default(context);
        g_main_context_unref(context);

        CTRACE(threads, "app private died for" << (void*)this);
    }

    void scheduleIdle(mox::IdleSource::Task task)
    {
        idle = std::move(task);
        if (!idle)
        {
            idle = [this]()
            {
                CTRACE(event, "Stopping TestCoreApp runloop");
                g_main_loop_quit(runLoop);
                return true;
            };
        }
        CTRACE(threads, "add idle source for" << (void*)this);
        idleSource = g_idle_source_new();
        g_source_set_callback(idleSource, &Private::defaultIdleFunc, this, nullptr);
        g_source_attach(idleSource, context);
        g_source_unref(idleSource);
    }
};

TestCoreApp::TestCoreApp()
    : d(new Private)
{
    m_instance = this;
}

TestCoreApp::~TestCoreApp()
{
    m_instance = nullptr;
}

TestCoreApp* TestCoreApp::instance()
{
    FATAL(m_instance, "Invalid test app instance");
    return m_instance;
}

void TestCoreApp::exit()
{
    g_main_loop_quit(d->runLoop);
}

void TestCoreApp::run()
{
    g_main_loop_run(d->runLoop);
}

void TestCoreApp::runOnce()
{
    d->scheduleIdle(nullptr);
    g_main_loop_run(d->runLoop);
}

void TestCoreApp::runOnce(mox::IdleSource::Task exitTask)
{
    d->scheduleIdle(exitTask);
    g_main_loop_run(d->runLoop);
}

void TestCoreApp::addIdleTask(mox::IdleSource::Task idle)
{
    if (!d || !g_main_loop_is_running(d->runLoop))
    {
        return;
    }
    d->scheduleIdle(idle);
}
