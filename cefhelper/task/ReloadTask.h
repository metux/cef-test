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
