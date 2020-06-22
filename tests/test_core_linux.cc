#include <mox/config/platform_config.hpp>
#include <test_framework.h>

#include <glib.h>


struct IdleBundle
{
    GMainContext* context = nullptr;
    mox::IdleFunction idle;
    guint sourceId;

    explicit IdleBundle(GMainContext* context, mox::IdleFunction&& idle)
        : context(context)
        , idle(std::forward<mox::IdleFunction>(idle))
    {
        sourceId = g_idle_add(&IdleBundle::callback, gpointer(this));
    }

    static gboolean callback(gpointer userData)
    {
        // sanity check, stop if the source is no longer part of the context!
        auto bundle = static_cast<IdleBundle*>(userData);
        if (!g_main_context_find_source_by_id(bundle->context, bundle->sourceId))
        {
            // The idle source is no longer in the context.
            delete bundle;
            return FALSE;
        }
        if (bundle->idle())
        {
            delete bundle;
            return FALSE;
        }
        // Need to reschedule again.
        return TRUE;
    }
};

class TestCoreApp::Private
{
public:
    GMainContext* context = nullptr;
    GMainLoop* runLoop = nullptr;

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
        g_main_loop_unref(runLoop);
        g_main_context_pop_thread_default(context);
        g_main_context_unref(context);

        CTRACE(threads, "app private died for" << (void*)this);
    }

    void scheduleIdle(mox::IdleFunction task)
    {
        if (!task)
        {
            task = [this]()
            {
                CTRACE(event, "Stopping TestCoreApp runloop");
                g_main_loop_quit(runLoop);
                return true;
            };
            new IdleBundle(context, std::move(task));
        }
        else
        {
            new IdleBundle(context, std::move(task));
        }
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

void TestCoreApp::runOnce(mox::IdleFunction exitTask)
{
    d->scheduleIdle(std::move(exitTask));
    g_main_loop_run(d->runLoop);
}

void TestCoreApp::addIdleTask(mox::IdleFunction idle)
{
    if (!d || !g_main_loop_is_running(d->runLoop))
    {
        return;
    }
    d->scheduleIdle(std::move(idle));
    g_main_context_wakeup(d->context);
}
