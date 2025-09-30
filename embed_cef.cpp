#include <X11/Xlib.h>
#include <iostream>
#include <list>
#include <filesystem>

#include "include/capi/cef_app_capi.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/internal/cef_linux.h"

class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler {
public:
    SimpleHandler() : browser_count_(0) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        browser_ = browser;
        browser_count_++;
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        browser_ = nullptr;
        browser_count_--;

        if (browser_count_-- <= 0) {
            CefQuitMessageLoop();
        }
    }

private:
    CefRefPtr<CefBrowser> browser_;
    std::atomic<int> browser_count_;

    IMPLEMENT_REFCOUNTING(SimpleHandler);
};

class SimpleApp : public CefApp {
public:
    SimpleApp() {}
    IMPLEMENT_REFCOUNTING(SimpleApp);
};

int already_in = 0;

int main(int argc, char* argv[])
{
    /* check whether we're in a sub-process */
    for (int x=1; x<argc; x++) {
        if (strncmp(argv[x], "--type=", 7)==0) {
            already_in = 1;
        }
    }

    /* just pass control to CEF if we're in a subprocess */
    if (already_in) {
        CefMainArgs main_args(argc, argv);
        CefRefPtr<SimpleApp> app = new SimpleApp();
        int exit_code = CefExecuteProcess(main_args, app, nullptr);
        if (exit_code >= 0) return exit_code;
        return 0;
    }

    /* parse cmdline args */
    if (argc < 1) {
        fprintf(stderr, "something's wrong with args: argc = 0\n");
        return 1;
    }

    uint32_t parent_xid = 0;
    if (argc > 1) {
        parent_xid = atoi(argv[1]);
        printf("parsing window id: %s -- %ld\n", argv[1], parent_xid);
    }

    CefSettings settings;
    settings.no_sandbox = true;

    std::filesystem::path cwd  = std::filesystem::current_path();
    std::string resources_path = cwd.string();
    std::string locales_path   = (cwd / "locales").string();
    std::string cache_path     = (cwd / "cache").string();

    CefString(&settings.resources_dir_path).FromASCII(resources_path.c_str());
    CefString(&settings.locales_dir_path).FromASCII(locales_path.c_str());
    CefString(&settings.cache_path).FromASCII(cache_path.c_str());

    /* dont pass it our actual args */
    CefMainArgs main_args(1, argv);
    CefRefPtr<SimpleApp> app = new SimpleApp();
    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
        std::cerr << "CEF initialization failed" << std::endl;
        return 1;
    }

    // 3. WindowInfo: child mode
    CefWindowInfo window_info;
    window_info.SetAsChild(
        (CefWindowHandle)parent_xid,
        CefRect(0, 0, 800, 600));

    CefBrowserSettings browser_settings;

    // 4. Create the browser
    CefRefPtr<SimpleHandler> handler(new SimpleHandler());
    CefBrowserHost::CreateBrowser(window_info,
                                  handler,
                                  "file:///",
                                  browser_settings,
                                  nullptr,
                                  nullptr);

    // 5. Run the CEF message loop
    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
