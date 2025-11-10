class ExecuteScriptTask : public CefTask {
public:
    ExecuteScriptTask(CefRefPtr<CefBrowser> b, const std::string& code)
        : browser(b), script(code) {}
    void Execute() override {
        if (!browser || !browser->GetMainFrame())
            return;
        browser->GetMainFrame()->ExecuteJavaScript(script, "about:blank", 0);
    }

private:
    CefRefPtr<CefBrowser> browser;
    std::string script;
    IMPLEMENT_REFCOUNTING(ExecuteScriptTask);
};
