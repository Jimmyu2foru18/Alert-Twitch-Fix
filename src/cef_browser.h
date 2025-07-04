#pragma once

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/cef_load_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_display_handler.h>
#include <include/cef_request_handler.h>
#include <include/wrapper/cef_helpers.h>
#include <obs-module.h>
#include <graphics/graphics.h>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

// Forward declaration
struct ChromiumSource;

/**
 * CEF Application class that handles global CEF initialization and configuration.
 * This class sets up CEF with the necessary command line switches to prevent
 * background throttling and ensure continuous rendering.
 */
class CEFApp : public CefApp, public CefBrowserProcessHandler {
public:
    CEFApp();
    
    // CefApp methods
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
        return this;
    }
    
    // CefBrowserProcessHandler methods
    void OnContextInitialized() override;
    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                     CefRefPtr<CefCommandLine> command_line) override;
    
private:
    IMPLEMENT_REFCOUNTING(CEFApp);
};

/**
 * CEF Render Handler that manages off-screen rendering.
 * This class receives painted frames from CEF and converts them to OBS textures.
 */
class CEFRenderHandler : public CefRenderHandler {
public:
    explicit CEFRenderHandler(ChromiumSource* source);
    
    // CefRenderHandler methods
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                PaintElementType type,
                const RectList& dirtyRects,
                const void* buffer,
                int width,
                int height) override;
    
    /**
     * Update the render size for the browser.
     */
    void SetSize(int width, int height);
    
private:
    ChromiumSource* source_;
    int width_;
    int height_;
    std::mutex size_mutex_;
    
    IMPLEMENT_REFCOUNTING(CEFRenderHandler);
};

/**
 * CEF Load Handler that manages page loading events.
 */
class CEFLoadHandler : public CefLoadHandler {
public:
    explicit CEFLoadHandler(ChromiumSource* source);
    
    // CefLoadHandler methods
    void OnLoadStart(CefRefPtr<CefBrowser> browser,
                    CefRefPtr<CefFrame> frame,
                    TransitionType transition_type) override;
    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                  CefRefPtr<CefFrame> frame,
                  int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                    CefRefPtr<CefFrame> frame,
                    ErrorCode errorCode,
                    const CefString& errorText,
                    const CefString& failedUrl) override;
    
private:
    ChromiumSource* source_;
    
    IMPLEMENT_REFCOUNTING(CEFLoadHandler);
};

/**
 * CEF Life Span Handler that manages browser lifecycle events.
 */
class CEFLifeSpanHandler : public CefLifeSpanHandler {
public:
    explicit CEFLifeSpanHandler(ChromiumSource* source);
    
    // CefLifeSpanHandler methods
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame,
                      const CefString& target_url,
                      const CefString& target_frame_name,
                      WindowOpenDisposition target_disposition,
                      bool user_gesture,
                      const CefPopupFeatures& popupFeatures,
                      CefWindowInfo& windowInfo,
                      CefRefPtr<CefClient>& client,
                      CefBrowserSettings& settings,
                      CefRefPtr<CefDictionaryValue>& extra_info,
                      bool* no_javascript_access) override;
    
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    
private:
    ChromiumSource* source_;
    
    IMPLEMENT_REFCOUNTING(CEFLifeSpanHandler);
};

/**
 * CEF Client that coordinates all the handlers and manages the browser instance.
 */
class CEFClient : public CefClient {
public:
    explicit CEFClient(ChromiumSource* source);
    
    // CefClient methods
    CefRefPtr<CefRenderHandler> GetRenderHandler() override {
        return render_handler_;
    }
    
    CefRefPtr<CefLoadHandler> GetLoadHandler() override {
        return load_handler_;
    }
    
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return life_span_handler_;
    }
    
    /**
     * Get the render handler for external access.
     */
    CefRefPtr<CEFRenderHandler> GetCEFRenderHandler() {
        return render_handler_;
    }
    
private:
    ChromiumSource* source_;
    CefRefPtr<CEFRenderHandler> render_handler_;
    CefRefPtr<CEFLoadHandler> load_handler_;
    CefRefPtr<CEFLifeSpanHandler> life_span_handler_;
    
    IMPLEMENT_REFCOUNTING(CEFClient);
};

/**
 * Main CEF Browser wrapper class that manages a single browser instance.
 * This class handles browser creation, navigation, and cleanup.
 */
class CEFBrowser {
public:
    explicit CEFBrowser(ChromiumSource* source);
    ~CEFBrowser();
    
    /**
     * Initialize the browser with the specified URL and dimensions.
     */
    bool Initialize(const std::string& url, int width, int height);
    
    /**
     * Navigate to a new URL.
     */
    void LoadURL(const std::string& url);
    
    /**
     * Reload the current page.
     */
    void Reload();
    
    /**
     * Resize the browser viewport.
     */
    void Resize(int width, int height);
    
    /**
     * Check if the browser is valid and ready.
     */
    bool IsValid() const;
    
    /**
     * Get the current URL.
     */
    std::string GetURL() const;
    
    /**
     * Force a repaint of the browser.
     */
    void Invalidate();
    
    /**
     * Cleanup and close the browser.
     */
    void Close();
    
private:
    ChromiumSource* source_;
    CefRefPtr<CefBrowser> browser_;
    CefRefPtr<CEFClient> client_;
    bool initialized_;
    std::string current_url_;
    
    // Browser settings
    void ConfigureBrowserSettings(CefBrowserSettings& settings);
};

/**
 * Global CEF management functions.
 */
namespace CEFManager {
    /**
     * Initialize the CEF framework with anti-throttling settings.
     */
    bool Initialize();
    
    /**
     * Shutdown the CEF framework and cleanup resources.
     */
    void Shutdown();
    
    /**
     * Process CEF message loop work (should be called regularly).
     */
    void DoMessageLoopWork();
    
    /**
     * Check if CEF is initialized.
     */
    bool IsInitialized();
}