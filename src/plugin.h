#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <graphics/graphics.h>
#include <memory>
#include <string>

// Forward declarations
class CEFBrowser;
class CEFAudio;

/**
 * Main plugin class that manages the Chromium-based OBS source.
 * This class handles the lifecycle of the CEF browser instance and
 * coordinates between the browser rendering and OBS texture output.
 */
class ChromiumPlugin {
public:
    ChromiumPlugin();
    ~ChromiumPlugin();
    
    /**
     * Initialize the plugin and CEF framework.
     * @return true if initialization was successful
     */
    bool Initialize();
    
    /**
     * Shutdown the plugin and cleanup CEF resources.
     */
    void Shutdown();
    
    /**
     * Get the singleton instance of the plugin.
     */
    static ChromiumPlugin* GetInstance();
    
private:
    static ChromiumPlugin* instance_;
    bool initialized_;
    
    // CEF application and message loop management
    void InitializeCEF();
    void ShutdownCEF();
};

/**
 * Structure representing a Chromium source instance in OBS.
 * Each source can load a different URL and has its own browser instance.
 */
struct ChromiumSource {
    obs_source_t* source;
    
    // Browser management
    std::unique_ptr<CEFBrowser> browser;
    std::unique_ptr<CEFAudio> audio;
    
    // Source properties
    std::string url;
    int width;
    int height;
    bool force_continuous_playback;
    float volume;
    bool muted;
    
    // Rendering
    gs_texture_t* texture;
    pthread_mutex_t texture_mutex;
    
    // Audio
    obs_source_t* audio_source;
    
    ChromiumSource();
    ~ChromiumSource();
};

// OBS source callbacks
extern "C" {
    // Source lifecycle
    static void* chromium_source_create(obs_data_t* settings, obs_source_t* source);
    static void chromium_source_destroy(void* data);
    static void chromium_source_update(void* data, obs_data_t* settings);
    
    // Rendering
    static void chromium_source_video_tick(void* data, float seconds);
    static void chromium_source_video_render(void* data, gs_effect_t* effect);
    static uint32_t chromium_source_get_width(void* data);
    static uint32_t chromium_source_get_height(void* data);
    
    // Properties
    static obs_properties_t* chromium_source_get_properties(void* data);
    static void chromium_source_get_defaults(obs_data_t* settings);
    
    // Audio
    static void chromium_source_enum_active_sources(void* data, obs_source_enum_proc_t enum_callback, void* param);
}

// Plugin registration info
#define PLUGIN_NAME "Chromium Browser Source"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR "OBS Chromium Plugin"
#define PLUGIN_DESCRIPTION "Advanced browser source with continuous rendering using Chromium Embedded Framework"

// Source type ID
#define CHROMIUM_SOURCE_ID "chromium_browser_source"

// Default settings
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080
#define DEFAULT_URL "about:blank"
#define DEFAULT_VOLUME 1.0f
#define DEFAULT_FORCE_CONTINUOUS true