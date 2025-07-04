#include "cef_browser.h"
#include "chromium_source.h"
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_command_line.h>
#include <include/wrapper/cef_helpers.h>
#include <obs-module.h>
#include <util/platform.h>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <thread>
#include <chrono>

// Global CEF state
static bool g_cef_initialized = false;
static CefRefPtr<CEFApp> g_cef_app;
static std::thread g_message_loop_thread;
static bool g_shutdown_requested = false;

// CEFApp implementation
CEFApp::CEFApp() {
}

void CEFApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    blog(LOG_INFO, "[CEF] Context initialized successfully");
}

void CEFApp::OnBeforeCommandLineProcessing(const CefString& process_type,
                                         CefRefPtr<CefCommandLine> command_line) {
    // Add command line switches to prevent background throttling
    command_line->AppendSwitch("--disable-background-timer-throttling");
    command_line->AppendSwitch("--disable-renderer-backgrounding");
    command_line->AppendSwitch("--disable-backgrounding-occluded-windows");
    command_line->AppendSwitch("--disable-background-media-suspend");
    command_line->AppendSwitch("--disable-features=TranslateUI");
    command_line->AppendSwitch("--disable-ipc-flooding-protection");
    command_line->AppendSwitch("--enable-media-stream");
    command_line->AppendSwitch("--autoplay-policy=no-user-gesture-required");
    command_line->AppendSwitch("--enable-gpu");
    command_line->AppendSwitch("--enable-gpu-compositing");
    command_line->AppendSwitch("--enable-begin-frame-scheduling");
    
    // Disable web security for local development
    command_line->AppendSwitch("--disable-web-security");
    command_line->AppendSwitch("--allow-running-insecure-content");
    
    blog(LOG_INFO, "[CEF] Applied anti-throttling command line switches");
}

// CEFRenderHandler implementation
CEFRenderHandler::CEFRenderHandler(ChromiumSource* source)
    : source_(source), width_(DEFAULT_WIDTH), height_(DEFAULT_HEIGHT) {
}

void CEFRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    std::lock_guard<std::mutex> lock(size_mutex_);
    rect.x = 0;
    rect.y = 0;
    rect.width = width_;
    rect.height = height_;
}

void CEFRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                              PaintElementType type,
                              const RectList& dirtyRects,
                              const void* buffer,
                              int width,
                              int height) {
    if (type != PET_VIEW || !source_ || !buffer) {
        return;
    }
    
    // Update texture with new frame data
    pthread_mutex_lock(&source_->texture_mutex);
    
    obs_enter_graphics();
    
    // Create or recreate texture if size changed
    if (!source_->texture || 
        gs_texture_get_width(source_->texture) != (uint32_t)width ||
        gs_texture_get_height(source_->texture) != (uint32_t)height) {
        
        if (source_->texture) {
            gs_texture_destroy(source_->texture);
        }
        
        source_->texture = gs_texture_create(width, height, GS_BGRA, 1, nullptr, GS_DYNAMIC);
        if (!source_->texture) {
            blog(LOG_ERROR, "[CEF] Failed to create texture (%dx%d)", width, height);
            obs_leave_graphics();
            pthread_mutex_unlock(&source_->texture_mutex);
            return;
        }
    }
    
    // Update texture data
    if (source_->texture) {
        uint8_t* texture_data;
        uint32_t linesize;
        
        if (gs_texture_map(source_->texture, &texture_data, &linesize)) {
            // CEF provides BGRA data, which matches OBS expectations
            const uint8_t* src = static_cast<const uint8_t*>(buffer);
            const uint32_t src_linesize = width * 4; // 4 bytes per pixel (BGRA)
            
            for (int y = 0; y < height; ++y) {
                memcpy(texture_data + y * linesize, 
                       src + y * src_linesize, 
                       src_linesize);
            }
            
            gs_texture_unmap(source_->texture);
        }
    }
    
    obs_leave_graphics();
    pthread_mutex_unlock(&source_->texture_mutex);
}

void CEFRenderHandler::SetSize(int width, int height) {
    std::lock_guard<std::mutex> lock(size_mutex_);
    width_ = width;
    height_ = height;
}

// CEFLoadHandler implementation
CEFLoadHandler::CEFLoadHandler(ChromiumSource* source) : source_(source) {
}

void CEFLoadHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType transition_type) {
    if (frame->IsMain()) {
        blog(LOG_INFO, "[CEF] Started loading: %s", frame->GetURL().ToString().c_str());
    }
}

void CEFLoadHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode) {
    if (frame->IsMain()) {
        blog(LOG_INFO, "[CEF] Finished loading: %s (Status: %d)", 
             frame->GetURL().ToString().c_str(), httpStatusCode);
        
        // Force an initial repaint to ensure content is visible
        browser->GetHost()->Invalidate(PET_VIEW);
    }
}

void CEFLoadHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
    if (frame->IsMain()) {
        blog(LOG_WARNING, "[CEF] Load error: %s (Code: %d, URL: %s)", 
             errorText.ToString().c_str(), errorCode, failedUrl.ToString().c_str());
    }
}

// CEFLifeSpanHandler implementation
CEFLifeSpanHandler::CEFLifeSpanHandler(ChromiumSource* source) : source_(source) {
}

bool CEFLifeSpanHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
                                      bool* no_javascript_access) {
    // Block popups by default
    blog(LOG_INFO, "[CEF] Blocked popup: %s", target_url.ToString().c_str());
    return true;
}

void CEFLifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    blog(LOG_INFO, "[CEF] Browser created successfully");
}

void CEFLifeSpanHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    blog(LOG_INFO, "[CEF] Browser closing");
}

// CEFClient implementation
CEFClient::CEFClient(ChromiumSource* source) : source_(source) {
    render_handler_ = new CEFRenderHandler(source);
    load_handler_ = new CEFLoadHandler(source);
    life_span_handler_ = new CEFLifeSpanHandler(source);
}

// CEFBrowser implementation
CEFBrowser::CEFBrowser(ChromiumSource* source) 
    : source_(source), initialized_(false) {
    client_ = new CEFClient(source);
}

CEFBrowser::~CEFBrowser() {
    Close();
}

bool CEFBrowser::Initialize(const std::string& url, int width, int height) {
    if (initialized_) {
        return true;
    }
    
    if (!CEFManager::IsInitialized()) {
        blog(LOG_ERROR, "[CEF] Cannot create browser: CEF not initialized");
        return false;
    }
    
    // Configure browser settings
    CefBrowserSettings browser_settings;
    ConfigureBrowserSettings(browser_settings);
    
    // Configure window info for off-screen rendering
    CefWindowInfo window_info;
    window_info.SetAsWindowless(nullptr);
    
    // Update render handler size
    client_->GetCEFRenderHandler()->SetSize(width, height);
    
    // Create the browser
    browser_ = CefBrowserHost::CreateBrowserSync(
        window_info, client_, url, browser_settings, nullptr, nullptr);
    
    if (!browser_) {
        blog(LOG_ERROR, "[CEF] Failed to create browser");
        return false;
    }
    
    current_url_ = url;
    initialized_ = true;
    
    blog(LOG_INFO, "[CEF] Browser initialized successfully with URL: %s", url.c_str());
    return true;
}

void CEFBrowser::LoadURL(const std::string& url) {
    if (!IsValid()) {
        return;
    }
    
    browser_->GetMainFrame()->LoadURL(url);
    current_url_ = url;
    blog(LOG_INFO, "[CEF] Loading URL: %s", url.c_str());
}

void CEFBrowser::Reload() {
    if (!IsValid()) {
        return;
    }
    
    browser_->Reload();
    blog(LOG_INFO, "[CEF] Reloading browser");
}

void CEFBrowser::Resize(int width, int height) {
    if (!IsValid()) {
        return;
    }
    
    client_->GetCEFRenderHandler()->SetSize(width, height);
    browser_->GetHost()->WasResized();
    browser_->GetHost()->Invalidate(PET_VIEW);
}

bool CEFBrowser::IsValid() const {
    return initialized_ && browser_ && !browser_->GetHost()->IsWindowRenderingDisabled();
}

std::string CEFBrowser::GetURL() const {
    if (IsValid()) {
        return browser_->GetMainFrame()->GetURL().ToString();
    }
    return current_url_;
}

void CEFBrowser::Invalidate() {
    if (IsValid()) {
        browser_->GetHost()->Invalidate(PET_VIEW);
    }
}

void CEFBrowser::Close() {
    if (browser_) {
        browser_->GetHost()->CloseBrowser(true);
        browser_ = nullptr;
    }
    initialized_ = false;
}

void CEFBrowser::ConfigureBrowserSettings(CefBrowserSettings& settings) {
    // Enable web security
    settings.web_security = STATE_DISABLED;
    
    // Enable JavaScript
    settings.javascript = STATE_ENABLED;
    settings.javascript_close_windows = STATE_DISABLED;
    settings.javascript_access_clipboard = STATE_DISABLED;
    
    // Enable plugins
    settings.plugins = STATE_ENABLED;
    
    // Background color (transparent)
    settings.background_color = CefColorSetARGB(0, 0, 0, 0);
    
    // Frame rate
    settings.windowless_frame_rate = 60;
}

// CEFManager implementation
namespace CEFManager {
    
bool Initialize() {
    if (g_cef_initialized) {
        return true;
    }
    
    blog(LOG_INFO, "[CEF] Initializing CEF framework");
    
    // Get the current executable path
    QString app_path = QApplication::applicationDirPath();
    QString cef_path = app_path + "/cef";
    
    // CEF main arguments
    CefMainArgs main_args;
    
    // CEF settings
    CefSettings settings;
    settings.no_sandbox = true;
    settings.multi_threaded_message_loop = false; // We'll handle the message loop
    settings.windowless_rendering_enabled = true;
    settings.background_color = CefColorSetARGB(0, 0, 0, 0);
    
    // Set paths
    CefString(&settings.browser_subprocess_path) = (cef_path + "/cef_subprocess.exe").toStdString();
    CefString(&settings.resources_dir_path) = (cef_path + "/Resources").toStdString();
    CefString(&settings.locales_dir_path) = (cef_path + "/Resources/locales").toStdString();
    
    // Cache directory
    QString cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/obs-chromium-plugin";
    QDir().mkpath(cache_dir);
    CefString(&settings.cache_path) = cache_dir.toStdString();
    
    // Log settings
    settings.log_severity = LOGSEVERITY_WARNING;
    CefString(&settings.log_file) = (cache_dir + "/cef.log").toStdString();
    
    // Create CEF app
    g_cef_app = new CEFApp();
    
    // Initialize CEF
    if (!CefInitialize(main_args, settings, g_cef_app, nullptr)) {
        blog(LOG_ERROR, "[CEF] Failed to initialize CEF");
        return false;
    }
    
    g_cef_initialized = true;
    g_shutdown_requested = false;
    
    // Start message loop thread
    g_message_loop_thread = std::thread([]() {
        blog(LOG_INFO, "[CEF] Message loop thread started");
        
        while (!g_shutdown_requested) {
            CefDoMessageLoopWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        blog(LOG_INFO, "[CEF] Message loop thread ended");
    });
    
    blog(LOG_INFO, "[CEF] CEF framework initialized successfully");
    return true;
}

void Shutdown() {
    if (!g_cef_initialized) {
        return;
    }
    
    blog(LOG_INFO, "[CEF] Shutting down CEF framework");
    
    // Signal shutdown
    g_shutdown_requested = true;
    
    // Wait for message loop thread to finish
    if (g_message_loop_thread.joinable()) {
        g_message_loop_thread.join();
    }
    
    // Shutdown CEF
    CefShutdown();
    
    g_cef_initialized = false;
    g_cef_app = nullptr;
    
    blog(LOG_INFO, "[CEF] CEF framework shut down successfully");
}

void DoMessageLoopWork() {
    if (g_cef_initialized) {
        CefDoMessageLoopWork();
    }
}

bool IsInitialized() {
    return g_cef_initialized;
}

} // namespace CEFManager