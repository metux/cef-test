
CefRefPtr<CefClient> createCefClient(std::string idx, std::string webhook);

class CreateBrowserTask: public CefTask {
public:
    CreateBrowserTask(std::string idx, uint32_t parent_xid, int width, int height, std::string url, std::string webhook)
        : _idx(idx), _parent_xid(parent_xid), _width(width), _height(height), _url(url), _webhook(webhook)
    {
        BrowserInfo inf = {
            .width = width,
            .height = height,
        };

        browser_info[idx] = inf;
    }

    static CefWindowInfo make_window_info(uint32_t parent_xid, int width, int height) {
        CefWindowInfo wi;
        wi.SetAsChild(
            (CefWindowHandle)parent_xid,
            CefRect(0, 0, width, height));
#ifdef USE_ALLOY
        wi.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
#endif
        return wi;
    }

    void Execute() override {
        CefBrowserHost::CreateBrowser(make_window_info(_parent_xid, _width, _height),
                                      createCefClient(_idx, _webhook),
                                      _url,
                                      CefBrowserSettings(),
                                      nullptr,
                                      nullptr);
    }

private:
    uint32_t _parent_xid;
    std::string _idx;
    int _width;
    int _height;
    std::string _url;
    std::string _webhook;
    IMPLEMENT_REFCOUNTING(CreateBrowserTask);
};

void taskCreate(std::string idx, uint32_t parent_xid, int width, int height, std::string url, std::string webhook) {
    CefPostTask(TID_UI, new CreateBrowserTask(idx, parent_xid, width, height, url, webhook));
}
