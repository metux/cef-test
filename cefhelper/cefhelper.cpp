#include <X11/Xlib.h>
#include <libgen.h>

#include <iostream>

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

#define DUMP_BROWSER_PTR(tag, browser) \
    fprintf(stderr, "[%s] browser=%p host=%p window=%p\n", \
            tag, (void*)(browser.get()), \
            (void*)(browser->GetHost().get()), \
            (void*)(browser->GetHost()->GetWindowHandle()));

static std::vector<CefRefPtr<CefBrowser>> browsers(10);

static inline void browsers_makeroom(int _idx) {
    if (browsers.size() < _idx+1)
        browsers.resize(_idx+1, nullptr);
}

static CefRefPtr<CefBrowser> mainBrowser;
static std::atomic<int> browser_count;

class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefKeyboardHandler {
public:
    SimpleHandler(int idx) : _idx(idx) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;  // MUST return the LifeSpan handler
    }

    void DUMP(CefRefPtr<CefBrowser> b, const char* tag) {
        if(!b) {
            fprintf(stderr, "[%s] browser=null\n", tag);
            return;
        }
        fprintf(stderr, "[%s] raw=%p id=%d host=%p window=%p\n",
                tag, (void*)b.get(), b->GetIdentifier(),
                (void*)b->GetHost().get(),
                (void*)b->GetHost()->GetWindowHandle());
    }

    bool DoClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        DUMP(browser, "DoClose");

        fprintf(stderr, "SimpleHandler::DoClose()\n");

        // *** FORCE CLOSE ***
        // true = destroy the browser even if the client returned false.
        // This makes the destruction synchronous with the UI thread
        // and guarantees the Views widget is destroyed *before* the
        // parent XID disappears.
        browser->GetHost()->CloseBrowser(true);

        fprintf(stderr, "SimpleHandler::DoClose() after CloseBrowser()\n");

        // Return true to tell CEF we have already taken care of closing.
        // (Returning false would let CEF try to close again → double-free.)
        return true;
//    return false;
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        DUMP(browser, "OnAfterCreated");

        browser_count++;
        browsers_makeroom(_idx);
        if (browsers[_idx] != nullptr)
            fprintf(stderr, "WARNING: browser slot %d already taken\n", _idx);
        browsers[_idx] = browser;
        fprintf(stderr, "--> assigned %p to browser slot %d\n", browser.get(), _idx);
        fprintf(stderr, "==> NOW %p\n", browsers[_idx].get());
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        DUMP(browser, "OnBeforeClose");

//        browser_count--;
        browsers[_idx] = nullptr;

#if 0
        if (browser_count <= 0) {
            CefQuitMessageLoop();
        }
#endif
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
        CEF_REQUIRE_UI_THREAD();
        DUMP(browser, "OnBeforePopup");
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
        CEF_REQUIRE_UI_THREAD();
        DUMP(browser, "OnPreKeyEvent");

        fprintf(stderr, "OnPreKeyEvent() browsers[1]=%p\n", browsers[1].get());

        /* FIXME: differenciate between CTRL vs CTRL-SHIFT */
        if (event.type == KEYEVENT_RAWKEYDOWN &&
            (event.modifiers & EVENTFLAG_CONTROL_DOWN)) {

            switch (event.windows_key_code) {
//                case 'N': /* CTRL-N new tab */
                case 'T': /* CTRL-T new window */
                case 'B': /* CTRL-B bookmarks window */
                case 'D': /* CTRL-D add bookmark */
//                case 'W': /* CTRL-W close window */
                case 'W':
                    fprintf(stderr, "OnPreKeyEvent() calling ->CloseBrowser(true)\n");
                    browser->GetHost()->CloseBrowser(true);
                break;
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
    int _idx;
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
        command_line->AppendSwitch("single-process");

        command_line->AppendSwitchWithValue("enable-features", "PartitionAllocBackupRefPtr,PartitionAllocDanglingPtr:mode/log_only/type/cross_task");

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

        // force gtk in order to avoid crashes in "views"
//        command_line->AppendSwitchWithValue("disable-features", "Views");
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

int cefhelper_run()
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

    CefRunMessageLoop();
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

class CloseTask: public CefTask {
public:
    CloseTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        if (!browser->GetHost()) {
            fprintf(stderr, "browser->GetHost() returned NULL\n");
            return;
        }
        browser->GetHost()->CloseBrowser(false);
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(CloseTask);
};

class CreateBrowserTask: public CefTask {
public:
    CreateBrowserTask(int idx, uint32_t parent_xid, int width, int height, const char *url)
        : _idx(idx), _parent_xid(parent_xid), _width(width), _height(height), _url(url) {}
    void Execute() override {
        CefBrowserHost::CreateBrowser(make_window_info(_parent_xid, _width, _height),
                                      new SimpleHandler(_idx),
                                      _url,
                                      make_browser_settings(),
                                      nullptr,
                                      nullptr);
    }

private:
    uint32_t _parent_xid;
    int _idx;
    int _width;
    int _height;
    std::string _url;
    IMPLEMENT_REFCOUNTING(CreateBrowserTask);
};

void cefhelper_loadurl(int idx, const char *url)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %d\n", idx);
        return;
    }
    CefPostTask(TID_UI, new LoadURLTask(browsers[idx], url));
}

void cefhelper_reload(int idx)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %d\n", idx);
        return;
    }
    CefPostTask(TID_UI, new ReloadTask(browsers[idx]));
}

void cefhelper_stopload(int idx)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %d\n", idx);
        return;
    }
    CefPostTask(TID_UI, new StopLoadTask(browsers[idx]));
}

void cefhelper_goback(int idx)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %d\n", idx);
        return;
    }
    CefPostTask(TID_UI, new GoBackTask(browsers[idx]));
}

void cefhelper_goforward(int idx)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %d\n", idx);
        return;
    }
    CefPostTask(TID_UI, new GoForwardTask(browsers[idx]));
}

void cefhelper_close(int idx)
{
    browsers_makeroom(idx);
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to close empty slot %d\n", idx);
        return;
    }
    fprintf(stderr, "X closing browser #%d -- %p\n", idx, browsers[idx].get());
    CefPostTask(TID_UI, new CloseTask(browsers[idx]));
}

void cefhelper_closeall(void)
{
    cefhelper_close(0);
#if 0
    for (int x=0; x<browsers.size(); x++) {
        if (browsers[x]) {
            fprintf(stderr, "closing browser #%d\n", x);
            CefPostTask(TID_UI, new CloseTask(browsers[x]));
            fprintf(stderr, "sent close message\n");
            browsers[x] = nullptr;
            fprintf(stderr, "cleared my own ref\n");
        }
    }
#endif
}

int cefhelper_create(int idx, uint32_t parent_xid, int width, int height, const char *url)
{
    browsers_makeroom(idx);
    if (browsers[idx] != nullptr) {
        fprintf(stderr, "WARNING: create: slot %d already taken\n", idx);
        return -1;
    }
    CefPostTask(TID_UI, new CreateBrowserTask(idx, parent_xid, width, height, url));
    return 0;
}
