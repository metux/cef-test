class StopLoadTask : public CefTask {
public:
    StopLoadTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->StopLoad();
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(StopLoadTask);
};
