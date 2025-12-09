
#include "../cefhelper_priv.h"

class ReloadTask : public CefTask {
public:
    ReloadTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;

        auto frame = browser->GetMainFrame();
        auto url = frame->GetURL().ToString();
        frame->LoadURL(url);
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(ReloadTask);
};

void taskReload(CefBrowserRef b) {
    CefPostTask(TID_UI, new ReloadTask(b));
}
