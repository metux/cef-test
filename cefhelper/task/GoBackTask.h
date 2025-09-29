class GoBackTask: public CefTask {
public:
    GoBackTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoBack();
    }

    static void Do(CefBrowserRef browser) {
        CefPostTask(TID_UI, new GoBackTask(browser));
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(GoBackTask);
};
