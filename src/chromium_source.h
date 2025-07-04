#pragma once

#include "plugin.h"
#include <obs-module.h>
#include <obs-properties.h>
#include <graphics/graphics.h>
#include <util/threading.h>
#include <memory>
#include <string>

// Forward declarations
class CEFBrowser;
class CEFAudio;
struct ChromiumSource;

/**
 * Chromium Source implementation for OBS Studio.
 * This class manages the lifecycle of a browser-based source,
 * including rendering, audio, and user interface properties.
 */
class ChromiumSourceImpl {
public:
    explicit ChromiumSourceImpl(obs_source_t* source);
    ~ChromiumSourceImpl();
    
    /**
     * Initialize the source with settings.
     */
    bool Initialize(obs_data_t* settings);
    
    /**
     * Update source settings.
     */
    void Update(obs_data_t* settings);
    
    /**
     * Video tick callback for frame updates.
     */
    void VideoTick(float seconds);
    
    /**
     * Render the source video content.
     */
    void VideoRender(gs_effect_t* effect);
    
    /**
     * Get the source width.
     */
    uint32_t GetWidth() const;
    
    /**
     * Get the source height.
     */
    uint32_t GetHeight() const;
    
    /**
     * Enumerate active audio sources.
     */
    void EnumActiveSources(obs_source_enum_proc_t enum_callback, void* param);
    
    /**
     * Get the OBS source handle.
     */
    obs_source_t* GetSource() const;
    
private:
    obs_source_t* obs_source_;
    
    // Browser management
    std::unique_ptr<CEFBrowser> browser_;
    std::unique_ptr<CEFAudio> audio_;
    
    // Source properties
    std::string url_;
    int width_;
    int height_;
    bool force_continuous_playback_;
    float volume_;
    bool muted_;
    bool auto_reload_;
    int reload_interval_;
    
    // Rendering
    gs_texture_t* texture_;
    pthread_mutex_t texture_mutex_;
    
    // Timing
    float last_reload_time_;
    
    // Helper methods
    void LoadSettings(obs_data_t* settings);
    void CreateBrowser();
    void DestroyBrowser();
    void UpdateBrowserSize();
    void ReloadBrowser();
};

/**
 * Property modification callbacks for the source UI.
 */
namespace ChromiumSourceProperties {
    /**
     * URL property modification callback.
     */
    bool url_modified(obs_properties_t* props, obs_property_t* property,
                     obs_data_t* settings);
    
    /**
     * Reload button callback.
     */
    bool reload_button_clicked(obs_properties_t* props, obs_property_t* property,
                              void* data);
    
    /**
     * Size preset selection callback.
     */
    bool size_preset_modified(obs_properties_t* props, obs_property_t* property,
                             obs_data_t* settings);
    
    /**
     * Custom size toggle callback.
     */
    bool custom_size_modified(obs_properties_t* props, obs_property_t* property,
                             obs_data_t* settings);
    
    /**
     * Auto reload toggle callback.
     */
    bool auto_reload_modified(obs_properties_t* props, obs_property_t* property,
                             obs_data_t* settings);
}

/**
 * Size preset definitions for common resolutions.
 */
struct SizePreset {
    const char* name;
    int width;
    int height;
};

// Common size presets
static const SizePreset SIZE_PRESETS[] = {
    {"Custom", 0, 0},
    {"1920x1080 (Full HD)", 1920, 1080},
    {"1280x720 (HD)", 1280, 720},
    {"1366x768 (WXGA)", 1366, 768},
    {"1600x900 (HD+)", 1600, 900},
    {"2560x1440 (QHD)", 2560, 1440},
    {"3840x2160 (4K UHD)", 3840, 2160},
    {"800x600 (SVGA)", 800, 600},
    {"1024x768 (XGA)", 1024, 768},
    {"1440x900 (WXGA+)", 1440, 900}
};

#define NUM_SIZE_PRESETS (sizeof(SIZE_PRESETS) / sizeof(SIZE_PRESETS[0]))

/**
 * Property identifiers for settings.
 */
#define PROP_URL "url"
#define PROP_WIDTH "width"
#define PROP_HEIGHT "height"
#define PROP_SIZE_PRESET "size_preset"
#define PROP_CUSTOM_SIZE "custom_size"
#define PROP_FORCE_CONTINUOUS "force_continuous"
#define PROP_VOLUME "volume"
#define PROP_MUTED "muted"
#define PROP_AUTO_RELOAD "auto_reload"
#define PROP_RELOAD_INTERVAL "reload_interval"
#define PROP_RELOAD_BUTTON "reload_button"
#define PROP_ADVANCED_GROUP "advanced_group"

/**
 * Default property values.
 */
#define DEFAULT_SIZE_PRESET 1  // 1920x1080
#define DEFAULT_CUSTOM_SIZE false
#define DEFAULT_AUTO_RELOAD false
#define DEFAULT_RELOAD_INTERVAL 300  // 5 minutes

/**
 * Property constraints.
 */
#define MIN_WIDTH 100
#define MAX_WIDTH 7680
#define MIN_HEIGHT 100
#define MAX_HEIGHT 4320
#define MIN_RELOAD_INTERVAL 10   // 10 seconds
#define MAX_RELOAD_INTERVAL 3600 // 1 hour

/**
 * Localization text keys.
 */
#define TEXT_URL "URL"
#define TEXT_URL_TOOLTIP "The URL to load in the browser"
#define TEXT_SIZE_PRESET "Size Preset"
#define TEXT_SIZE_PRESET_TOOLTIP "Select a common resolution or choose custom"
#define TEXT_CUSTOM_SIZE "Custom Size"
#define TEXT_CUSTOM_SIZE_TOOLTIP "Enable custom width and height settings"
#define TEXT_WIDTH "Width"
#define TEXT_WIDTH_TOOLTIP "Browser viewport width in pixels"
#define TEXT_HEIGHT "Height"
#define TEXT_HEIGHT_TOOLTIP "Browser viewport height in pixels"
#define TEXT_FORCE_CONTINUOUS "Force Continuous Playback"
#define TEXT_FORCE_CONTINUOUS_TOOLTIP "Keep browser active even when source is hidden"
#define TEXT_VOLUME "Volume"
#define TEXT_VOLUME_TOOLTIP "Audio volume level (0-100%)"
#define TEXT_MUTED "Muted"
#define TEXT_MUTED_TOOLTIP "Mute audio output"
#define TEXT_AUTO_RELOAD "Auto Reload"
#define TEXT_AUTO_RELOAD_TOOLTIP "Automatically reload the page at specified intervals"
#define TEXT_RELOAD_INTERVAL "Reload Interval (seconds)"
#define TEXT_RELOAD_INTERVAL_TOOLTIP "Time between automatic reloads"
#define TEXT_RELOAD_BUTTON "Reload Page"
#define TEXT_RELOAD_BUTTON_TOOLTIP "Manually reload the current page"
#define TEXT_ADVANCED_GROUP "Advanced Settings"