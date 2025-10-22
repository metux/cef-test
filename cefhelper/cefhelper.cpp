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
#include "include/cef_frame.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/internal/cef_linux.h"

static CefRefPtr<CefBrowser> mainBrowser;

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
        fprintf(stderr, "A OnAfterCreated() now have %d browsers\n", browser_count_.load());
        browser_count_++;
        fprintf(stderr, "B OnAfterCreated() now have %d browsers\n", browser_count_.load());
        mainBrowser = browser;
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
//        browser_ = nullptr;

        fprintf(stderr, "A OnBeforeClose() now have %d browsers\n", browser_count_.load());

        browser_count_--;

        fprintf(stderr, "B OnBeforeClose() now have %d browsers\n", browser_count_.load());

        if (browser_count_ <= 0) {
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

class SimpleApp : public CefApp,
                  public CefBrowserProcessHandler {
public:
    SimpleApp() {}

    // Return myself as browser process handler
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }

    virtual void OnBeforeCommandLineProcessing(
            const CefString& process_type,
            CefRefPtr<CefCommandLine> command_line) override
    {
        // Disable Segmentation Platform
        command_line->AppendSwitch("disable-segmentation-platform");

        // Disable ML / Optimization systems
        command_line->AppendSwitch("disable-machine-learning");
        command_line->AppendSwitch("disable-optimization-guide");

        // Disable reporting/telemetry
        command_line->AppendSwitch("disable-domain-reliability");
        command_line->AppendSwitch("disable-background-networking");

        // Disable spellcheck / translate
        command_line->AppendSwitch("disable-spell-checking");
        command_line->AppendSwitch("disable-translate");

        // Disable safe browsing (removes Google blocklists etc.)
        command_line->AppendSwitch("disable-client-side-phishing-detection");
        command_line->AppendSwitch("disable-component-update");
        command_line->AppendSwitch("safe-browsing-disable-auto-update");
        command_line->AppendSwitch("safebrowsing-disable-download-protection");

        // Disable metrics / crash reporting
        command_line->AppendSwitch("disable-metrics");
        command_line->AppendSwitch("disable-metrics-reporting");

        // Keep GPU minimal (optional)
        command_line->AppendSwitch("disable-software-rasterizer");
        command_line->AppendSwitch("disable-gpu-shader-disk-cache");

        // don't write trash to filesystem
        command_line->AppendSwitch("incognito");
    }

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
    CefString(&settings.cache_path).FromASCII(""); /* only in-memory */
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

    SimpleHandler* handler = new SimpleHandler();

    CefBrowserHost::CreateBrowser(make_window_info(parent_xid, width, height),
                                  handler,
                                  url,
                                  make_browser_settings(),
                                  nullptr,
                                  nullptr);

    CefBrowserHost::CreateBrowser(make_window_info(parent_xid, width, height),
                                  handler,
                                  url,
                                  make_browser_settings(),
                                  nullptr,
                                  nullptr);

    fprintf(stderr, "Calling CefRunMessageLoop()\n");
    CefRunMessageLoop();
    fprintf(stderr, "Returned from CefRunMessageLoop() ... calling CefShutdown()\n");
    CefShutdown();
    return 0;
}

class LoadURLTask : public CefTask {
public:
    LoadURLTask(CefRefPtr<CefBrowser> b, std::string u) : browser(b), url(u) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GetMainFrame()->LoadURL(url);
    }
private:
    CefRefPtr<CefBrowser> browser;
    std::string url;
    IMPLEMENT_REFCOUNTING(LoadURLTask);
};

class ReloadTask : public CefTask {
public:
    ReloadTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;

        auto frame = browser->GetMainFrame();
        auto url = frame->GetURL().ToString();
        frame->LoadURL(url);
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(ReloadTask);
};

class StopLoadTask : public CefTask {
public:
    StopLoadTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;

        browser->StopLoad();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(StopLoadTask);
};

class GoBackTask: public CefTask {
public:
    GoBackTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoBack();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(GoBackTask);
};

class GoForwardTask: public CefTask {
public:
    GoForwardTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoForward();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(GoForwardTask);
};

void cefhelper_loadurl(const char *url)
{
    CefPostTask(TID_UI, new LoadURLTask(mainBrowser, url));
}

void cefhelper_reload(void)
{
    CefPostTask(TID_UI, new ReloadTask(mainBrowser));
}

void cefhelper_stopload(void)
{
    CefPostTask(TID_UI, new StopLoadTask(mainBrowser));
}

void cefhelper_goback(void)
{
    CefPostTask(TID_UI, new GoBackTask(mainBrowser));
}

void cefhelper_goforward(void)
{
    CefPostTask(TID_UI, new GoForwardTask(mainBrowser));
}
