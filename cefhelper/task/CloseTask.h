
#include "../cefhelper_priv.h"

void taskClose(CefBrowserRef b) {
    // already dispatching itself to UI thread
    if (b && b->GetHost())
        b->GetHost()->CloseBrowser(true);
}
