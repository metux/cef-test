
#include "../cefhelper_priv.h"

class PrintTask : public CefTask {
public:
    PrintTask(CefBrowserRef b, bool show_dialog)
        : browser(b), show_dialog(show_dialog) {}

    void Execute() override {
        if (!browser || !browser->GetHost())
            return;

        if (show_dialog) {
            // Show native print dialog
            browser->GetHost()->Print();
        } else {
            // Silent print using default printer (no dialog)
            CefRefPtr<CefPrintHandler> print_handler = nullptr; // could implement custom margins etc.
            browser->GetHost()->Print();
        }
    }

private:
    CefBrowserRef browser;
    bool show_dialog;
    IMPLEMENT_REFCOUNTING(PrintTask);
};

void taskPrint(CefBrowserRef b) {
    CefPostTask(TID_UI, new PrintTask(b, true));
}
