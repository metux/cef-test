#include <X11/Xlib.h>
#include <libgen.h>

#include <iostream>

#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>

#include "cefhelper_priv.h"
#include "CefHelperApp.h"

#include "include/capi/cef_app_capi.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_frame.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/internal/cef_linux.h"

#define USE_ALLOY

static std::unordered_map<std::string,CefBrowserRef> browsers;
static std::unordered_map<std::string,BrowserInfo> browser_info;

#include "task/GoForwardTask.h"
#include "task/GoBackTask.h"
#include "task/CloseTask.h"
#include "task/CreateBrowserTask.h"
#include "task/ExecuteScriptTask.h"
#include "task/LoadURLTask.h"
#include "task/ReloadTask.h"
#include "task/StopLoadTask.h"
#include "task/PrintTask.h"
#include "task/RepaintTask.h"
#include "task/ResizeTask.h"
#include "task/ZoomTask.h"

class CefHelperHandler: public CefClient,
                        public CefLifeSpanHandler,
                        public CefKeyboardHandler,
                        public CefLoadHandler,
                        public CefDisplayHandler,
                        public CefRequestHandler,
                        public CefResourceRequestHandler {
public:
    CefHelperHandler(std::string idx, std::string webhook) : _idx(idx), _webhook(webhook) {}

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

    void DUMP(CefBrowserRef b, const char* tag) {
        if(!b) {
            fprintf(stderr, "[%s] browser=null\n", tag);
            return;
        }
        fprintf(stderr, "[%s] raw=%p id=%d host=%p window=%p\n",
                tag, (void*)b.get(), b->GetIdentifier(),
                (void*)b->GetHost().get(),
                (void*)b->GetHost()->GetWindowHandle());
    }

    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
        CefBrowserRef browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool is_navigation,
        bool is_download,
        const CefString& request_initiator,
        bool& disable_default_handling) override
    {
        return this;
    }

#ifdef USE_ALLOY
    bool OnCertificateError(CefRefPtr<CefBrowser> browser,
                            cef_errorcode_t cert_error,
                            const CefString& request_url,
                            CefRefPtr<CefSSLInfo> ssl_info,
                            CefRefPtr<CefCallback> callback) override {
        CEF_REQUIRE_UI_THREAD();

        // Log for debugging (optional)
        fprintf(stderr, "SSL Cert Error %d on %s — bypassing\n",
                cert_error, request_url.ToString().c_str());

        // Force continue: Ignores outdated/expired cert, no interstitial
        callback->Continue();

        // Return true: We've handled it (don't let CEF show interstitial)
        return true;
    }
#endif

    void OnStatusMessage(CefBrowserRef browser, const CefString& value) override
    {
        fprintf(stderr, "STATUS: %s\n", value.ToString().c_str());
    }

    void OnTitleChange(CefBrowserRef browser, const CefString& title)
    {
        fprintf(stderr, "TITLE: %s\n", title.ToString().c_str());
    }

    virtual void OnLoadingProgressChange(CefBrowserRef browser, double progress)
    {
        fprintf(stderr, "PROGRESS: %f\n", progress);
    }

    bool OnResourceResponse(
        CefBrowserRef browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        CefRefPtr<CefResponse> response) override
    {
        // not on UI thread !
        // doesn't seem to be called at all in error case

        int status = response->GetStatus();               // e.g. 200, 404, 500, 301...
        const CefString& url = request->GetURL();

        fprintf(stderr, "[HTTP %d] %s\n", status, url.ToString().c_str());

        if (frame->IsMain()) {
            fprintf(stderr, "=== MAIN FRAME STATUS: %d for browser %d ===\n", status, _idx);
        }
        return true;
    }

    void OnAfterCreated(CefBrowserRef browser) override {
        if (browsers[_idx] != nullptr)
            fprintf(stderr, "WARNING: OnAfterCreate() browser slot %s already taken\n", _idx);

        Window xid = (Window) browser->GetHost()->GetWindowHandle();
        browsers[_idx] = browser;
        browser_info[_idx].xid = xid;
        browser_info[_idx].browser = browser;

        auto style = browser->GetHost()->GetRuntimeStyle();

        fprintf(stderr, "OnAfterCreated(): browser %s child window: %x style: %d\n",
            _idx.c_str(), xid, style);

        browser->GetHost()->SetFocus(true);

        switch (style) {
            case CEF_RUNTIME_STYLE_DEFAULT:
                fprintf(stderr, "CEF_RUNTIME_STYLE_DEFAULT\n"); break;
            case CEF_RUNTIME_STYLE_ALLOY:
                fprintf(stderr, "CEF_RUNTIME_STYLE_ALLOY\n"); break;
            case CEF_RUNTIME_STYLE_CHROME:
                fprintf(stderr, "CEF_RUNTIME_STYLE_CHROME\n"); break;
            default:
                fprintf(stderr, "CEF_RUNTIME_STYLE_UNKNOWN\n"); break;
        }

        postEvent(
            "browser.ready",
            { { "xid", std::to_string(xid) } }
        );
    }

    void OnBeforeClose(CefBrowserRef browser) override {
        browsers.erase(_idx);
        browser_info.erase(_idx);
        postEvent("browser.close");
    }

    virtual bool OnBeforePopup(
        CefBrowserRef browser,
        CefRefPtr<CefFrame> frame,
        const CefString& target_url,
        const CefString& target_frame_name,
        CefLifeSpanHandler::WindowOpenDisposition target_disposition,
        bool user_gesture,
        const CefPopupFeatures& popupFeatures,
        CefWindowInfo& windowInfo,
        CefClientRef& client,
        CefBrowserSettings& settings,
        bool* no_javascript_access)
    {
        DUMP(browser, "OnBeforePopup");
        return true; // Returning true cancels popup creation
    }

    virtual bool OnPreKeyEvent(CefBrowserRef browser,
                               const CefKeyEvent& event,
                               CefEventHandle os_event,
                               bool* is_keyboard_shortcut) override
    {
#ifdef USE_ALLOY
        if (event.type == KEYEVENT_RAWKEYDOWN &&
            (event.modifiers & EVENTFLAG_ALT_DOWN)) {
            switch (event.windows_key_code) {
                case 37: /* VK_LEFT - Ctrl+Left = GoBack */
                    taskBack(browser);
                    return true;  // Consume (don't pass to page)
                case 39: /* VK_RIGHT - Ctrl+Right = GoForward */
                    taskForward(browser);
                    return true;
            }
        }
#endif /* USE_ALLOY */
        /* FIXME: differenciate between CTRL vs CTRL-SHIFT */
        if (event.type == KEYEVENT_RAWKEYDOWN &&
            (event.modifiers & EVENTFLAG_CONTROL_DOWN)) {
            switch (event.windows_key_code) {
                case 'N': /* CTRL-N new tab */
                case 'T': /* CTRL-T new window */
                case 'B': /* CTRL-B bookmarks window */
                case 'D': /* CTRL-D add bookmark */
//                case 'W': /* CTRL-W close window */
                case 'P': /* CTRL-P print */
                case 'J': /* CTRL-J downloads window */
                case 'M': /* CTRL-SHIFT-M switch user */
                case 'H': /* CTRL-H history window */
                    return true;
            }
        }

        return false;
    }

    bool OnBeforeBrowse(CefBrowserRef browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefRequest> request,
                        bool user_gesture,
                        bool is_redirect) override
    {
        postEvent(
            "navigation.started",
            { { "url", request->GetURL() } }
        );
        return false;
    }

    void OnLoadingStateChange(CefBrowserRef browser,
                              bool isLoading,
                              bool canGoBack,
                              bool canGoForward) override
    {
        if (!isLoading) {
            std::string current_url = browser->GetMainFrame()->GetURL().ToString();
            int current_status = 200; // FIXME

            fprintf(stderr, "[LOAD COMPLETE] browser=%p id=%d url=%s\n",
                    (void*)browser.get(),
                    browser->GetIdentifier(),
                    current_url.c_str());

            postEvent(
                "navigation.finished",
                { { "url", current_url }, { "httpStatus", std::to_string(current_status) } }
            );
        }
    }

    void OnLoadError(CefBrowserRef browser,
                     CefRefPtr<CefFrame> frame,
                     cef_errorcode_t errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override
    {
        fprintf(stderr, "[LOAD ERROR] code=%d err=\"%s\" url=\"%s\"\n",
            errorCode, errorText.ToString().c_str(), failedUrl.ToString().c_str());

        if (errorCode == ERR_BLOCKED_BY_CLIENT) {
            fprintf(stderr, "blocked by client\n");
//            frame->LoadURL(failedUrl);
            return;  // Abort default error handling
        }

        if (frame->IsMain() && errorCode == ERR_ABORTED) {
            fprintf(stderr, "OnLoadError() code=ERR_ABORTED\n");
            // This is the exact case: real 404 with body, but Chromium aborts commit
            // → reload the same URL once, but with bypass of the broken error-page logic
            frame->LoadURL(failedUrl);
            return;
        } else
            fprintf(stderr, "OnLoadError() other err code %d\n", errorCode);

        browser->StopLoad();
        // this is breaking history (can't go backward anymore)
        //        frame->LoadURL("data:text/html,<h1>Offline</h1>");
        postEvent(
            "navigation.failed",
            { { "url", failedUrl.ToString() }, { "reason", std::to_string(errorCode) } }
        );
    }

    // CefRenderProcessHandler
    virtual bool OnConsoleMessage(CefBrowserRef browser,
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
            "console.message", {
                { "message", message.ToString() },
                { "severity", sev },
                { "source", source.ToString() },
                { "line", std::to_string(line) }
        });

        // Return false = let CEF also log it internally if it wants
        // Return true  = suppress CEF's own logging
        return false;
    }

    virtual void OnLoadEnd(CefBrowserRef browser,
                           CefRefPtr<CefFrame> frame,
                           int httpStatusCode) override
    {
        std::string current_url = frame->GetURL().ToString();
        postEvent(
            "dom.contentLoaded",
            { { "url", frame->GetURL().ToString() } }
        );
    }

private:
    std::string _idx;
    std::string _webhook;
    IMPLEMENT_REFCOUNTING(CefHelperHandler);

    int postEvent(std::string name, std::string body) {
        return doHttpPost(
            _webhook + "/api/v1/windows/" + _idx + "/events/" + name,
            body);
    }

    int postEvent(std::string name) {
        return postEvent(name, "");
    }

    int postEvent(std::string name, std::unordered_map<std::string,std::string> data) {
        std::string s = "{ ";
        int x = 0;

        for (const auto& pair : data) {
            std::cout << pair.first << " = " << pair.second << '\n';
            if (x) s += ", ";
            s += "\""+pair.first+"\": \""+pair.second+"\"";
            x++;
        }

        s += " }";
        return postEvent(name, s);
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

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        curl_slist_free_all(hs);

        return 0;
    }
};

CefClientRef createCefClient(std::string idx, std::string webhook) {
    return new CefHelperHandler(idx, webhook);
}

bool check_cef_subprocess(int argc, char *argv[]) {
    /* check whether we're in a sub-process */
    for (int x=1; x<argc; x++) {
        if (strncmp(argv[x], "--type=", 7)==0)
            return true;
    }
    return false;
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
    CefString(&settings.root_cache_path).FromASCII(root_cache_path);
    settings.log_severity = LOGSEVERITY_VERBOSE;

    return settings;
}

int cefhelper_subprocess(int argc, char *argv[]) {
    CefMainArgs main_args(argc, argv);
    CefRefPtr<CefHelperApp> app = new CefHelperApp();
    return CefExecuteProcess(main_args, app, nullptr);
}

int cefhelper_run()
{
    /* dont pass it our actual args */
    CefRefPtr<CefHelperApp> app = new CefHelperApp();
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

void cefhelper_loadurl(const char *idx, const char *url)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    taskLoadURL(browsers[idx], url);
}

void cefhelper_reload(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    taskReload(browsers[idx]);
}

void cefhelper_stopload(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    taskStopLoad(browsers[idx]);
}

void cefhelper_goback(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    taskBack(browsers[idx]);
}

void cefhelper_goforward(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to access empty slot %s\n", idx);
        return;
    }
    taskForward(browsers[idx]);
}

void cefhelper_close(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to close empty slot %s\n", idx);
        return;
    }
    taskClose(browsers[idx]);
}

void cefhelper_repaint(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to repaint empty slot %s\n", idx);
        return;
    }
    taskRepaint(browsers[idx], browser_info[idx]);
}

void cefhelper_resize(const char *idx, int w, int h)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to resize empty slot %s\n", idx);
        return;
    }
    browser_info[idx].width = w;
    browser_info[idx].height = h;
    taskResize(browser_info[idx]);
}

void cefhelper_zoom(const char *idx, cefhelper_zoom_mode_t mode, double level)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: trying to zoom empty slot %s\n", idx);
        return;
    }

    fprintf(stderr, "zoom: idx=%s\n", idx);
    fprintf(stderr, "      mode=%d\n", mode);
    fprintf(stderr, "      level=%f\n", level);

    taskZoom(browser_info[idx].browser, mode, level);
}

void cefhelper_execjs(const char *idx, const char *code)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: execjs on empty slot %s\n", idx);
        return;
    }
    taskExecuteScript(browsers[idx], code);
}

void cefhelper_print(const char *idx)
{
    if (browsers[idx] == nullptr) {
        fprintf(stderr, "WARNING: print on empty slot %s\n", idx);
        return;
    }
    taskPrint(browsers[idx]);
}

void cefhelper_closeall(void)
{
    for (const auto& pair : browsers) {
        if (pair.second != nullptr) {
            taskClose(pair.second);
        }
    }
}

int cefhelper_create(const char *idx, uint32_t parent_xid, int width, int height, const char *url, const char *webhook)
{
    if (browsers[idx] != nullptr) {
        fprintf(stderr, "WARNING: create: slot %s already taken\n", idx);
        return -1;
    }
    taskCreate(idx, parent_xid, width, height, url, webhook);
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
