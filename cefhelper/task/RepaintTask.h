class RepaintTask: public CefTask {
public:
    RepaintTask(CefRefPtr<CefBrowser> b, BrowserInfo i) : _browser(b), _info(i) {}

    void Execute() override {
        Display *dpy = cef_get_xdisplay();
        Window win = (Window) _browser->GetHost()->GetWindowHandle();
        fprintf(stderr, "== RepaintTask: resizing window %x to %d:%d\n", win, _info.width, _info.height);

        XResizeWindow(dpy, win, _info.width + 1, _info.height + 1);
        XSync(dpy, False);
        usleep(10000);
        XResizeWindow(dpy, win, _info.width, _info.height);
        XSync(dpy, False);
    }

private:
    CefRefPtr<CefBrowser> _browser;
    BrowserInfo _info;
    IMPLEMENT_REFCOUNTING(RepaintTask);
};
