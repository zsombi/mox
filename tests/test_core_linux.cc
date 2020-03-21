#include <mox/config/platform_config.hpp>
#include <test_framework.h>

#include <glib.h>


class TestCoreApp::Private
{
public:
    GMainContext* context = nullptr;
    GMainLoop* runLoop = nullptr;

    Private()
    {
        context = g_main_context_default();
        FATAL(context, "The main context is not defined!");
        g_main_context_ref(context);
        runLoop = g_main_loop_new(context, false);
        g_main_context_push_thread_default(context);

    }
    ~Private()
    {
        g_main_context_pop_thread_default(context);
        g_main_loop_unref(runLoop);
        g_main_context_unref(context);
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
