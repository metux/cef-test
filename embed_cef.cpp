#include <X11/Xlib.h>
#include <iostream>

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
    SimpleHandler() {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        browser_ = browser;
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        browser_ = nullptr;
    }

private:
    CefRefPtr<CefBrowser> browser_;

    IMPLEMENT_REFCOUNTING(SimpleHandler);
};

// horrible hack
#define MYDIR "/home/nekrad/src/netcom/ceftest"

int main(int argc, char* argv[]) {
    // 1. Initialize CEF
    CefMainArgs main_args(argc, argv);
    CefRefPtr<CefApp> app = nullptr;

    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    if (exit_code >= 0) return exit_code;

    CefSettings settings;
    settings.no_sandbox = true;

    // Tell CEF exactly where resources are
    CefString(&settings.resources_dir_path).FromASCII(MYDIR "/third_party/cef/Resources");
//    CefString(&settings.locales_dir_path).FromASCII(MYDIR "/third_party/cef/Resources/locales");
//    CefString(&settings.locales_dir_path).FromASCII(MYDIR "/locales");

    CefString(&settings.root_cache_path).FromASCII(MYDIR "/cache");

    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
        std::cerr << "CEF initialization failed" << std::endl;
        return 1;
    }

//    CefInitialize(main_args, settings, app, nullptr);

    // 2. Parent window (XID) from host app
    // For example, read from argv or obtained via IPC
    Window parent_xid = strtoul(argv[1], nullptr, 0);

    // 3. WindowInfo: child mode
    CefWindowInfo window_info;
    int width = 800, height = 600;
    window_info.SetAsChild((CefWindowHandle)parent_xid, CefRect(0, 0, width, height));

    CefBrowserSettings browser_settings;

    // 4. Create the browser
    CefRefPtr<SimpleHandler> handler(new SimpleHandler());
    CefBrowserHost::CreateBrowser(window_info, handler,
                                  "https://example.com",
                                  browser_settings,
                                  nullptr, nullptr);

    // 5. Run the CEF message loop
    CefRunMessageLoop();

    CefShutdown();
    return 0;
}
