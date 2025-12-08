
#include "../cefhelper_priv.h"

class GoForwardTask: public CefTask {
public:
    GoForwardTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoForward();
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(GoForwardTask);
};

void taskForward(CefBrowserRef b) {
    CefPostTask(TID_UI, new GoForwardTask(b));
}
