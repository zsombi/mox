#include <mox/config/platform_config.hpp>
#include <test_framework.h>

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include "../src/platforms/darwin/mac_util.h"

class TestCoreApp::Private
{
public:
    Private() = default;
    ~Private() = default;

    mox::mac::CFType<CFRunLoopRef> runLoop = mox::mac::CFType<CFRunLoopRef>::constructFromGet(CFRunLoopGetCurrent());
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
