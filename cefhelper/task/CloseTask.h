class CloseTask: public CefTask {
public:
    CloseTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        if (!browser->GetHost()) {
            fprintf(stderr, "browser->GetHost() returned NULL\n");
            return;
        }
        browser->GetHost()->CloseBrowser(false);
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(CloseTask);
};
