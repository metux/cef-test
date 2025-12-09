
#include "../cefhelper_priv.h"

class ResizeTask: public CefTask {
public:
    ResizeTask(BrowserInfo i) : _info(i) {}

    void Execute() override {
        Display *dpy = cef_get_xdisplay();
        Window win = (Window) _info.browser->GetHost()->GetWindowHandle();
        XResizeWindow(dpy, win, _info.width, _info.height);
        XSync(dpy, False);
    }

private:
    BrowserInfo _info;
    IMPLEMENT_REFCOUNTING(ResizeTask);
};

void taskResize(BrowserInfo i) {
    CefPostTask(TID_UI, new ResizeTask(i));
}
