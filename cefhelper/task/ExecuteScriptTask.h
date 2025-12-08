class ExecuteScriptTask : public CefTask {
public:
    ExecuteScriptTask(CefBrowserRef b, const std::string& code)
        : browser(b), script(code) {}
    void Execute() override {
        if (!browser || !browser->GetMainFrame())
            return;
        browser->GetMainFrame()->ExecuteJavaScript(script, "about:blank", 0);
    }

private:
    CefBrowserRef browser;
    std::string script;
    IMPLEMENT_REFCOUNTING(ExecuteScriptTask);
};

void taskExecuteScript(CefBrowserRef b, const std::string& code) {
    CefPostTask(TID_UI, new ExecuteScriptTask(b, code));
}
