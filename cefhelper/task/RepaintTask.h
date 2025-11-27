class RepaintTask: public CefTask {
public:
    RepaintTask(CefRefPtr<CefBrowser> b) : browser(b) {}
    void Execute() override {
        auto host = browser->GetHost();
         if (!browser || !host)
                return;

        // Triple-hammer: Notify + WasResized + Invalidate (Ozone needs the sequence)
        host->NotifyMoveOrResizeStarted();  // Reset compositor state
        host->WasResized();                 // Trigger layout/damage
        host->Invalidate(PET_VIEW);         // Full invalidation

        fprintf(stderr, "Forced repaint on browser %d after parent restore\n", 
            browser->GetIdentifier());

        // Get current bounds (from parent process side, pass via IPC if needed)
        int curr_w = 140/* query parent child rect width */;
        int curr_h = 90/* query parent child rect height */;

        // 1px toggle (harmless, WM ignores micro-changes)
        CefRect tiny = {0, 0, curr_w + 1, curr_h};
        host->SetBounds(tiny);  // Triggers real XConfigureWindow on CEF child
        usleep(10 * 1000);      // Micro-delay for X11 sync (or use XSync)

        // Restore exact
        CefRect exact = {0, 0, curr_w, curr_h};
        host->SetBounds(exact);
        host->WasResized();

        // Bonus: Force focus (wakes compositor)
        host->SetFocus(true);
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(RepaintTask);
};
