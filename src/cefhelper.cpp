#include <X11/Xlib.h>
#include <libgen.h>

#include <iostream>
#include <list>
#include <filesystem>

#include <stdbool.h>

#include "cefhelper.h"

#include "include/capi/cef_app_capi.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/internal/cef_linux.h"

class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefKeyboardHandler {
public:
    SimpleHandler() : browser_count_(0) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;  // MUST return the LifeSpan handler
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

    virtual bool OnBeforePopup(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& target_url,
        const CefString& target_frame_name,
        CefLifeSpanHandler::WindowOpenDisposition target_disposition,
        bool user_gesture,
        const CefPopupFeatures& popupFeatures,
        CefWindowInfo& windowInfo,
        CefRefPtr<CefClient>& client,
        CefBrowserSettings& settings,
        bool* no_javascript_access)
    {
        // Prevent any new popup
        return true; // Returning true cancels popup creation
    }

    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override {
        return this;
    }

    virtual bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                               const CefKeyEvent& event,
                               CefEventHandle os_event,
                               bool* is_keyboard_shortcut) override
    {
        /* FIXME: differenciate between CTRL vs CTRL-SHIFT */
        if (event.type == KEYEVENT_RAWKEYDOWN &&
            (event.modifiers & EVENTFLAG_CONTROL_DOWN)) {

            switch (event.windows_key_code) {
                case 'N': /* CTRL-N new tab */
                case 'T': /* CTRL-T new window */
                case 'B': /* CTRL-B bookmarks window */
                case 'D': /* CTRL-D add bookmark */
                case 'W': /* CTRL-W close window */
                case 'P': /* CTRL-P print */
                case 'J': /* CTRL-J downloads window */
                case 'M': /* CTRL-SHIFT-M switch user */
                case 'H': /* CTRL-H history window */
                    return true;
            }
        }

        return false;
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

bool check_cef_subprocess(int argc, char *argv[]) {
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

static CefBrowserSettings make_browser_settings(void) {
    return CefBrowserSettings();
}

static CefSettings make_settings(void) {
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink failed");
        exit(1);
    }
    exe_path[len] = '\0'; // Null-terminate
    dirname(exe_path);

    char locales_path[PATH_MAX] = { 0 };
    snprintf(locales_path, sizeof(locales_path)-1, "%s/locales", exe_path);

    char root_cache_path[PATH_MAX] = { 0 };
    snprintf(root_cache_path, sizeof(root_cache_path)-1, "/tmp/cef_cache/%d", getpid());

    CefSettings settings;
    settings.no_sandbox = false;
    CefString(&settings.resources_dir_path).FromASCII(exe_path);
    CefString(&settings.locales_dir_path).FromASCII(locales_path);
    CefString(&settings.cache_path).FromASCII(NULL); /* only in-memory */
    // CefString(&settings.log_file).FromASCII("cef.log");
    CefString(&settings.root_cache_path).FromASCII(root_cache_path);
    settings.log_severity = LOGSEVERITY_VERBOSE;

    return settings;
}

int cefhelper_subprocess(int argc, char *argv[]) {
    CefMainArgs main_args(argc, argv);
    CefRefPtr<SimpleApp> app = new SimpleApp();
    return CefExecuteProcess(main_args, app, nullptr);
}

int cefhelper_run(
    uint32_t parent_xid,
    int width,
    int height,
    const char *url)
{
    /* dont pass it our actual args */
    CefRefPtr<SimpleApp> app = new SimpleApp();
    if (!CefInitialize(CefMainArgs(0, NULL),
                       make_settings(),
                       app.get(),
                       nullptr)) {
        std::cerr << "CEF initialization failed" << std::endl;
        return 1;
    }

    CefBrowserHost::CreateBrowser(make_window_info(parent_xid, width, height),
                                  new SimpleHandler(),
                                  url,
                                  make_browser_settings(),
                                  nullptr,
                                  nullptr);

    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
