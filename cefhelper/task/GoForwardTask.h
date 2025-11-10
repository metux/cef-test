class GoForwardTask: public CefTask {
public:
    GoForwardTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoForward();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(GoForwardTask);
};
