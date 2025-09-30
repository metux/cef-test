#include <X11/Xlib.h>
#include <libgen.h>

#include <iostream>
#include <list>
#include <filesystem>

#include <stdbool.h>

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

static bool check_cef_subprocess(int argc, char *argv[]) {
    /* check whether we're in a sub-process */
    for (int x=1; x<argc; x++) {
        if (strncmp(argv[x], "--type=", 7)==0)
            return true;
    }
    return false;
}

static CefWindowInfo make_window_info(uint32_t parent_xid, int width, int height) {
    CefWindowInfo wi;
    wi.SetAsChild(
        (CefWindowHandle)parent_xid,
        CefRect(0, 0, width, height));
    return wi;
}

static CefSettings make_settings(void) {
    char exe_path[4096];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink failed");
        exit(1);
    }
    exe_path[len] = '\0'; // Null-terminate
    dirname(exe_path);

    std::string basepath = std::string(exe_path);
    std::string resources_path = basepath;
    std::string locales_path = basepath + "/locales";

    CefSettings settings;
    settings.no_sandbox = false;
    CefString(&settings.resources_dir_path).FromASCII(resources_path.c_str());
    CefString(&settings.locales_dir_path).FromASCII(locales_path.c_str());
    CefString(&settings.cache_path).FromASCII(NULL); /* only in-memory */
    // CefString(&settings.log_file).FromASCII("cef.log");
    settings.log_severity = LOGSEVERITY_VERBOSE;

    return settings;
}

int main(int argc, char* argv[])
{
    /* just pass control to CEF if we're in a subprocess */
    if (check_cef_subprocess(argc, argv)) {
        CefMainArgs main_args(argc, argv);
        CefRefPtr<SimpleApp> app = new SimpleApp();
        int exit_code = CefExecuteProcess(main_args, app, nullptr);
        if (exit_code >= 0) return exit_code;
        return 0;
    }

    uint32_t parent_xid = 0;
    if (argc > 1) {
        parent_xid = strtol(argv[1], NULL, 16);
        printf("parsing window id: %lX\n", parent_xid);
    }

    /* dont pass it our actual args */
    CefRefPtr<SimpleApp> app = new SimpleApp();
    if (!CefInitialize(CefMainArgs(1, argv),
                       make_settings(),
                       app.get(),
                       nullptr)) {
        std::cerr << "CEF initialization failed" << std::endl;
        return 1;
    }

    CefBrowserSettings browser_settings;
    CefBrowserHost::CreateBrowser(make_window_info(parent_xid, 800, 600),
                                  new SimpleHandler(),
                                  "file:///",
                                  browser_settings,
                                  nullptr,
                                  nullptr);

    // 5. Run the CEF message loop
    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
