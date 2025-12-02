
#include "../cefhelper_priv.h"

class ZoomTask : public CefTask {
public:
    ZoomTask(CefRefPtr<CefBrowser> b, cefhelper_zoom_mode_t mode, double delta = 0.0)
        : _browser(b), _mode(mode), _delta(delta) {}

    void Execute() override {
        fprintf(stderr, "ZoomTask::Execute()\n");
        if (!_browser)
            return;

        auto host = _browser->GetHost();
        if (!host)
            return;

        switch (_mode) {
            case CEFHELPER_ZOOM_SET:
                fprintf(stderr, "zoom set\n");
                host->SetZoomLevel(_delta);
            break;
            case CEFHELPER_ZOOM_IN:
                fprintf(stderr, "zoom in: %f\n", (_delta ? _delta : 0.25));
                host->SetZoomLevel(
                    host->GetZoomLevel() + (_delta ? _delta : 0.25));
            break;
            case CEFHELPER_ZOOM_OUT:
                fprintf(stderr, "zoom out: %f\n", (_delta ? _delta : 0.25));
                host->SetZoomLevel(
                    host->GetZoomLevel() - (_delta ? _delta : 0.25));
            break;
        }
    }

private:
    CefRefPtr<CefBrowser> _browser;
    cefhelper_zoom_mode_t _mode;
    double _delta;
    IMPLEMENT_REFCOUNTING(ZoomTask);
};
