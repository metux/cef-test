class CloseTask: public CefTask {
public:
    CloseTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        if (!browser->GetHost())
            return;
        browser->GetHost()->CloseBrowser(false);
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(CloseTask);
};
