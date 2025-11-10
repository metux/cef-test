class GoBackTask: public CefTask {
public:
    GoBackTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoBack();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(GoBackTask);
};
