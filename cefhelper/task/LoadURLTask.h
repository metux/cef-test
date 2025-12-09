
#include "../cefhelper_priv.h"

class LoadURLTask : public CefTask {
public:
    LoadURLTask(CefBrowserRef b, std::string u) : browser(b), url(u) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GetMainFrame()->LoadURL(url);
    }
private:
    CefBrowserRef browser;
    std::string url;
    IMPLEMENT_REFCOUNTING(LoadURLTask);
};

void taskLoadURL(CefBrowserRef b, std::string u) {
    CefPostTask(TID_UI, new LoadURLTask(b, u));
}
