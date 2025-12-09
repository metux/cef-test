
class CreateBrowserTask: public CefTask {
public:
    CreateBrowserTask(CefClientRef client, uint32_t parent_xid,
                      int width, int height, std::string url)
        : _client(client), _parent_xid(parent_xid), _width(width),
          _height(height), _url(url)
    { }

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
                                      _client,
                                      _url,
                                      CefBrowserSettings(),
                                      nullptr,
                                      nullptr);
    }

private:
    CefClientRef _client;
    uint32_t _parent_xid;
    int _width;
    int _height;
    std::string _url;
    IMPLEMENT_REFCOUNTING(CreateBrowserTask);
};

void taskCreate(CefClientRef client, uint32_t parent_xid,
                int width, int height, std::string url) {
    CefPostTask(TID_UI,
                new CreateBrowserTask(client, parent_xid, width, height, url));
}
