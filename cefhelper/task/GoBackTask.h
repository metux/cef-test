class GoBackTask: public CefTask {
public:
    GoBackTask(CefBrowserRef b) : browser(b) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GoBack();
    }

private:
    CefBrowserRef browser;
    IMPLEMENT_REFCOUNTING(GoBackTask);
};

void taskBack(CefBrowserRef b) {
    CefPostTask(TID_UI, new GoBackTask(b));
}
