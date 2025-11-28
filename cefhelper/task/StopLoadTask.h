class StopLoadTask : public CefTask {
public:
    StopLoadTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->StopLoad();
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(StopLoadTask);
};
