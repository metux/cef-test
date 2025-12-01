
#include "../cefhelper_priv.h"

class ZoomTask : public CefTask {
public:
    enum class Op { Set, In, Out, Reset };

    ZoomTask(CefRefPtr<CefBrowser> b, Op op, double delta = 0.0)
        : browser(b), operation(op), delta(delta) {}

    void Execute() override {
        if (!browser || !browser->GetHost())
            return;

        if (operation == Op::Reset) {
            browser->GetHost()->SetZoomLevel(0.0);
        } else if (operation == Op::Set) {
            browser->GetHost()->SetZoomLevel(delta);
        } else {
            double current = browser->GetHost()->GetZoomLevel();
            double step = 0.25;  // you can make this configurable
            if (operation == Op::In)
                browser->GetHost()->SetZoomLevel(current + step);
            else
                browser->GetHost()->SetZoomLevel(current - step);
        }
    }

private:
    CefRefPtr<CefBrowser> browser;
    Op operation;
    double delta;
    IMPLEMENT_REFCOUNTING(ZoomTask);
};
