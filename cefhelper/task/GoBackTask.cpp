
#include "../cefhelper_priv.h"

__attribute__((visibility("default"), used))
class GoBackTask: public CefTask {
public:
    GoBackTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoBack();
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING_EXPORT(GoBackTask);
};

void taskBack(CefBrowserRef b) {
    CefPostTask(TID_UI, new GoBackTask(b));
}
