#include <X11/Xlib.h>
#include <libgen.h>

#include <iostream>

#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>

#include "cefhelper.h"

#include "include/capi/cef_app_capi.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_frame.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/internal/cef_linux.h"

static std::unordered_map<std::string,CefRefPtr<CefBrowser>> browsers;


class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefKeyboardHandler,
                      public CefLoadHandler,
                      public CefDisplayHandler,
                      public CefRequestHandler {
public:
    SimpleHandler(std::string idx, std::string webhook) : _idx(idx), _webhook(webhook) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

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

        // *** FORCE CLOSE ***
        // true = destroy the browser even if the client returned false.
        // This makes the destruction synchronous with the UI thread
        // and guarantees the Views widget is destroyed *before* the
        // parent XID disappears.
        browser->GetHost()->CloseBrowser(true);

        // Return true to tell CEF we have already taken care of closing.
        // (Returning false would let CEF try to close again → double-free.)
        return true;
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();

        if (browsers[_idx] != nullptr)
            fprintf(stderr, "WARNING: browser slot %s already taken\n", _idx);
        browsers[_idx] = browser;

        postEvent("browser.ready", "");
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();

        browsers.erase(_idx);

        postEvent("browser.close", "");
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
        return true; // Returning true cancels popup creation
    }

    virtual bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser,
                               const CefKeyEvent& event,
                               CefEventHandle os_event,
                               bool* is_keyboard_shortcut) override
    {
        CEF_REQUIRE_UI_THREAD();

        /* FIXME: differenciate between CTRL vs CTRL-SHIFT */
        if (event.type == KEYEVENT_RAWKEYDOWN &&
            (event.modifiers & EVENTFLAG_CONTROL_DOWN)) {

            switch (event.windows_key_code) {
                case 'N': /* CTRL-N new tab */
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

    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefRequest> request,
                        bool user_gesture,
                        bool is_redirect) override
    {
        std::string url = request->GetURL();
        fprintf(stderr, "[OnBeforeBrowse] url=%s\n", url.c_str());
        if (url.find("chrome://network-error/") != std::string::npos ||
            url.find("chrome-error://") != std::string::npos) {
            fprintf(stderr, "[BLOCKED] error page: %s\n", url.c_str());
            return true;  // CANCEL
        }

        postEvent("navigation.started", "{ \"url\": \""+url+"\" }");
        return false;
    }

    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                              bool isLoading,
                              bool canGoBack,
                              bool canGoForward) override
    {
        CEF_REQUIRE_UI_THREAD();

        if (!isLoading) {
            std::string current_url = browser->GetMainFrame()->GetURL().ToString();
            int current_status = 200; // FIXME

            fprintf(stderr, "[LOAD COMPLETE] browser=%p id=%d url=%s\n",
                    (void*)browser.get(),
                    browser->GetIdentifier(),
                    current_url.c_str());

            postEvent(
                "navigation.finished",
                "{ \"url\": \""+current_url+"\", \"httpStatus\": "+std::to_string(current_status)+" }"
            );
        }
    }

    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     cef_errorcode_t errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override
    {
        browser->StopLoad();
        // Optionally load custom blank page
        // this is breaking history (can't go backward anymore)
        //        frame->LoadURL("data:text/html,<h1>Offline</h1>");
        postEvent(
            "navigation.failed",
            "{ \"url\": \""+failedUrl.ToString()+"\", \"reason\": \"\" }"
        );
    }

    // CefRenderProcessHandler
    virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  cef_log_severity_t level,
                                  const CefString& message,
                                  const CefString& source,
                                  int line) override
    {
        std::string sev = "";
        switch (level) {
            case LOGSEVERITY_INFO:    sev = "INFO"; break;
            case LOGSEVERITY_WARNING: sev = "WARN"; break;
            case LOGSEVERITY_ERROR:   sev = "ERROR"; break;
            case LOGSEVERITY_FATAL:   sev = "FATAL"; break;
            default:                  sev = "DEBUG"; break;
        }

        postEvent(
            "console.message",
            "{ \"message\": \""+message.ToString()
                +"\", \"severity\": \""+sev+"\", \"source\": \""
                +source.ToString()+"\", \"line\": "+std::to_string(line)+" }"
        );

        // Return false = let CEF also log it internally if it wants
        // Return true  = suppress CEF's own logging
        return false;
    }

    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           int httpStatusCode) override
    {
        fprintf(stderr, "OnLoadEnd: %d\n", httpStatusCode);

        std::string current_url = frame->GetURL().ToString();

        postEvent(
            "dom.contentLoaded",
            "{ \"url\": \""+current_url+"\" }"
        );
    }

private:
    std::string _idx;
    std::string _webhook;
    IMPLEMENT_REFCOUNTING(SimpleHandler);

    int postEvent(std::string name, std::string body) {
        return doHttpPost(
            _webhook + "/api/v1/windows/" + _idx + "/events/" + name,
            body);
    }

    int doHttpPost(std::string url, std::string body) {
        CURL *curl = curl_easy_init();
        if (!curl)
            return 1;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());

        struct curl_slist *hs = curl_slist_append(NULL, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

        printf("webhook url %s...\n\n", url.c_str());
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            printf("\n\nHTTP Status: %ld\n", http_code);
        }

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        curl_slist_free_all(hs);

        return 0;
    }
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

        // disable GCM / Firebase
        command_line->AppendSwitch("disable-features=WebPush,GCM");
        command_line->AppendSwitch("disable-sync");
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

#include "task/CloseTask.h"
#include "task/CreateBrowserTask.h"
#include "task/ExecuteScriptTask.h"
#include "task/LoadURLTask.h"
#include "task/ReloadTask.h"
#include "task/GoForwardTask.h"
#include "task/StopLoadTask.h"
#include "task/GoBackTask.h"

void cefhelper_loadurl(const char *idx, const char *url)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new LoadURLTask(browsers[idx], url));
}

void cefhelper_reload(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new ReloadTask(browsers[idx]));
}

void cefhelper_stopload(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new StopLoadTask(browsers[idx]));
}

void cefhelper_goback(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new GoBackTask(browsers[idx]));
}

void cefhelper_goforward(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new GoForwardTask(browsers[idx]));
}

void cefhelper_close(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to close empty slot %s\n", idx);
        return;
    }
    fprintf(stderr, "X closing browser #%s -- %p\n", idx, browsers[idx].get());
    CefPostTask(TID_UI, new CloseTask(browsers[idx]));
}

void cefhelper_execjs(const char *idx, const char *code)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: execjs on empty slot %s\n", idx);
        return;
    }
    CefPostTask(TID_UI, new ExecuteScriptTask(browsers[idx], code));
}

void cefhelper_closeall(void)
{
    fprintf(stderr, "cefhelper_closeall()\n");
    for (const auto& pair : browsers) {
        if (pair.second != nullptr) {
            fprintf(stderr, "closeall browser %s\n", pair.first.c_str());
            CefPostTask(TID_UI, new CloseTask(pair.second));
        }
    }
}

int cefhelper_create(const char *idx, uint32_t parent_xid, int width, int height, const char *url, const char *webhook)
{
    fprintf(stderr, "create: url=%s webhook=%s\n", url, webhook);
    if (browsers[idx] != nullptr) {
        fprintf(stderr, "WARNING: create: slot %s already taken\n", idx);
        return -1;
    }
    CefPostTask(TID_UI, new CreateBrowserTask(idx, parent_xid, width, height, strdup(url), strdup(webhook)));
    return 0;
}

static void safe_str_append(char *dest, size_t size, const char *src) {
    size_t len = strlen(dest);
    if (len < size - 1) {
        strncat(dest, src, size - len - 1);
    }
}

int cefhelper_list(char *buf, size_t bufmax)
{
    buf[0] = 0;

    for (const auto& pair : browsers) {
        if (pair.second != nullptr) {
            safe_str_append(buf, bufmax, pair.first.c_str());
            safe_str_append(buf, bufmax, "\n");
        }
    }
    return 0;
}
