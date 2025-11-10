class LoadURLTask : public CefTask {
public:
    LoadURLTask(CefRefPtr<CefBrowser> b, std::string u) : browser(b), url(u) {}
    void Execute() override {
        if (!browser)
            return;
        browser->GetMainFrame()->LoadURL(url);
    }
private:
    CefRefPtr<CefBrowser> browser;
    std::string url;
    IMPLEMENT_REFCOUNTING(LoadURLTask);
};
