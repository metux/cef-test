#ifndef _CEFHELPER_PRIV_H
#define _CEFHELPER_PRIV_H

#include "include/cef_task.h"
#include "include/cef_browser.h"
#include "include/internal/cef_ptr.h"

#include "cefhelper.h"

typedef CefRefPtr<CefBrowser> CefBrowserRef;
typedef CefRefPtr<CefClient> CefClientRef;

class BrowserInfo {
    public:
        CefBrowserRef browser;
        Window xid;
        int width;
        int height;
};

void taskClose(CefBrowserRef b);
void taskCreate(CefClientRef client, uint32_t parent_xid, int width, int height, std::string url);
void taskExecuteScript(CefBrowserRef b, const std::string& code);
void taskBack(CefBrowserRef b);
void taskForward(CefBrowserRef b);
void taskLoadURL(CefBrowserRef b, std::string u);
void taskPrint(CefBrowserRef b);
void taskReload(CefBrowserRef b);
void taskRepaint(CefBrowserRef b, BrowserInfo i);
void taskResize(BrowserInfo i);
void taskStopLoad(CefBrowserRef b);
void taskZoom(CefBrowserRef b, cefhelper_zoom_mode_t m, double l);

#endif /* _CEFHELPER_PRIV_H */
