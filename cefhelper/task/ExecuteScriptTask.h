
#include "../cefhelper_priv.h"

void taskExecuteScript(CefBrowserRef b, const std::string& code) {
    if (b && b->GetMainFrame())
        b->GetMainFrame()->ExecuteJavaScript(code, "about:blank", 0);
}
