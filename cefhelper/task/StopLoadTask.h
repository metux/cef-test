
#include "../cefhelper_priv.h"

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
    IMPLEMENT_REFCOUNTING(StopLoadTask);
};

void taskStopLoad(CefBrowserRef b) {
    CefPostTask(TID_UI, new StopLoadTask(b));
}
