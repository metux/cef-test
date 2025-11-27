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
    }

private:
    CefRefPtr<CefBrowser> browser;
    IMPLEMENT_REFCOUNTING(RepaintTask);
};
