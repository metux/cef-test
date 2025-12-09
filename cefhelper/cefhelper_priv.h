#ifndef _CEFHELPER_PRIV_H
#define _CEFHELPER_PRIV_H

#include "include/cef_task.h"
#include "include/cef_browser.h"

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

#endif /* _CEFHELPER_PRIV_H */
