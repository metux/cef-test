class CreateBrowserTask: public CefTask {
public:
    CreateBrowserTask(const char *idx, uint32_t parent_xid, int width, int height, const char *url, const char *webhook)
        : _idx(idx), _parent_xid(parent_xid), _width(width), _height(height), _url(url), _webhook(webhook) {}
    void Execute() override {
        CefBrowserHost::CreateBrowser(make_window_info(_parent_xid, _width, _height),
                                      new SimpleHandler(_idx, _webhook),
                                      _url,
                                      make_browser_settings(),
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
