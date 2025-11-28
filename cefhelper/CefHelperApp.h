#include "include/cef_app.h"

class CefHelperApp: public CefApp,
                    public CefBrowserProcessHandler {
public:
    CefHelperApp() {}

    // Return myself as browser process handler
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }

    virtual void OnBeforeCommandLineProcessing(
            const CefString& process_type,
            CefRefPtr<CefCommandLine> command_line) override
    {
        command_line->AppendSwitch("single-process");

        command_line->AppendSwitchWithValue("enable-features", "PartitionAllocBackupRefPtr,PartitionAllocDanglingPtr:mode/log_only/type/cross_task");

        // Disable Segmentation Platform
        command_line->AppendSwitch("disable-segmentation-platform");

        // Disable ML / Optimization systems
        command_line->AppendSwitch("disable-machine-learning");
        command_line->AppendSwitch("disable-optimization-guide");

        // Disable reporting/telemetry
        command_line->AppendSwitch("disable-domain-reliability");
        command_line->AppendSwitch("disable-background-networking");

        // Disable spellcheck / translate
        command_line->AppendSwitch("disable-spell-checking");
        command_line->AppendSwitch("disable-translate");

        // Disable safe browsing (removes Google blocklists etc.)
        command_line->AppendSwitch("disable-client-side-phishing-detection");
        command_line->AppendSwitch("disable-component-update");
        command_line->AppendSwitch("safe-browsing-disable-auto-update");
        command_line->AppendSwitch("safebrowsing-disable-download-protection");

        // Disable metrics / crash reporting
        command_line->AppendSwitch("disable-metrics");
        command_line->AppendSwitch("disable-metrics-reporting");

        // Keep GPU minimal (optional)
        command_line->AppendSwitch("disable-software-rasterizer");
        command_line->AppendSwitch("disable-gpu-shader-disk-cache");

        // don't write trash to filesystem
//        command_line->AppendSwitch("incognito");

        // disable GCM / Firebase
        command_line->AppendSwitch("disable-features=WebPush,GCM");
        command_line->AppendSwitch("disable-sync");

        command_line->AppendSwitch("disable-web-security");
    }

    IMPLEMENT_REFCOUNTING(CefHelperApp);
};
