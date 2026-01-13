
#include "../cefhelper_priv.h"

void taskClose(CefBrowserRef b) {
    // already dispatching itself to UI thread
    if (b) {
        fprintf(stderr, "taskClose() browser ptr != null\n");
        if (b->GetHost())
            fprintf(stderr, "taskClose() GetHost() returns != null\n");
        else
            fprintf(stderr, "taskClose() GetHost() returns NULL\n");
    } else
        fprintf(stderr, "taskCloes() browser ptr == NULL\n");

    if (b && b->GetHost()) {
        fprintf(stderr, "taskClose() calling b->GetHost()->CloseBrowser()\n");
        b->GetHost()->CloseBrowser(true);
    }
}
