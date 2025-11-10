class ReloadTask : public CefTask {
public:
    ReloadTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;

        auto frame = browser->GetMainFrame();
        auto url = frame->GetURL().ToString();
        frame->LoadURL(url);
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(ReloadTask);
};
