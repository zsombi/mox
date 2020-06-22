#include <mox/config/platform_config.hpp>
#include <test_framework.h>

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include "../src/platforms/darwin/mac_util.h"
#include "../src/platforms/darwin/event_dispatcher.h"

class TestCoreApp::Private
{
public:
    Private()
    {
        activityWatcher.addToMode(kCFRunLoopCommonModes);
    }
    ~Private() = default;

    void processIdle(CFRunLoopActivity activity)
    {
        if (activity == kCFRunLoopBeforeWaiting)
        {
            if (idle)
            {
                idle();
            }
        }
        else
        {
            TRACE("what the heck!!?");
        }
    }

    mox::mac::CFType<CFRunLoopRef> runLoop = mox::mac::CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent());
    mox::RunLoopObserver<Private> activityWatcher{this, &Private::processIdle, kCFRunLoopBeforeWaiting};
    mox::IdleFunction idle;
};

TestCoreApp::TestCoreApp()
    : d(std::make_unique<Private>())
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
    CFRunLoopStop(d->runLoop);
}

void TestCoreApp::run()
{
    CFRunLoopRun();
}

void TestCoreApp::runOnce()
{
    d->idle = [this]()
    {
        this->exit();
        return true;
    };
    CFRunLoopRun();
}

void TestCoreApp::runOnce(mox::IdleFunction exiter)
{
    if (!exiter)
    {
        runOnce();
    }
    else
    {
        d->idle = exiter;
        CFRunLoopRun();
    }
}

void TestCoreApp::addIdleTask(mox::IdleFunction idle)
{
    if (!d)
    {
        return;
    }
    d->idle = std::move(idle);
    CFRunLoopWakeUp(d->runLoop);
}

