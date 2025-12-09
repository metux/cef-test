
#include "../cefhelper_priv.h"

__attribute__((visibility("default"), used))
class StopLoadTask : public CefTask {
public:
    StopLoadTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->StopLoad();
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING_EXPORT(StopLoadTask);
};

void taskStopLoad(CefBrowserRef b) {
    CefPostTask(TID_UI, new StopLoadTask(b));
}
