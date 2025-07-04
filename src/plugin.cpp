#include "plugin.h"
#include "cef_browser.h"
#include "cef_audio.h"
#include "chromium_source.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <graphics/graphics.h>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>

// Module information
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-chromium-plugin", "en-US")

// Global plugin instance
ChromiumPlugin* ChromiumPlugin::instance_ = nullptr;

// Module lifecycle functions
bool obs_module_load(void) {
    blog(LOG_INFO, "[Chromium Plugin] Loading plugin version %s", PLUGIN_VERSION);
    
    // Initialize the plugin
    ChromiumPlugin* plugin = ChromiumPlugin::GetInstance();
    if (!plugin->Initialize()) {
        blog(LOG_ERROR, "[Chromium Plugin] Failed to initialize plugin");
        return false;
    }
    
    // Register the source type
    struct obs_source_info chromium_source_info = {};
    chromium_source_info.id = CHROMIUM_SOURCE_ID;
    chromium_source_info.type = OBS_SOURCE_TYPE_INPUT;
    chromium_source_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE;
    chromium_source_info.get_name = [](void*) -> const char* { return PLUGIN_NAME; };
    chromium_source_info.create = chromium_source_create;
    chromium_source_info.destroy = chromium_source_destroy;
    chromium_source_info.update = chromium_source_update;
    chromium_source_info.video_tick = chromium_source_video_tick;
    chromium_source_info.video_render = chromium_source_video_render;
    chromium_source_info.get_width = chromium_source_get_width;
    chromium_source_info.get_height = chromium_source_get_height;
    chromium_source_info.get_properties = chromium_source_get_properties;
    chromium_source_info.get_defaults = chromium_source_get_defaults;
    chromium_source_info.enum_active_sources = chromium_source_enum_active_sources;
    chromium_source_info.icon_type = OBS_ICON_TYPE_BROWSER;
    
    obs_register_source(&chromium_source_info);
    
    blog(LOG_INFO, "[Chromium Plugin] Plugin loaded successfully");
    return true;
}

void obs_module_unload(void) {
    blog(LOG_INFO, "[Chromium Plugin] Unloading plugin");
    
    ChromiumPlugin* plugin = ChromiumPlugin::GetInstance();
    if (plugin) {
        plugin->Shutdown();
        delete plugin;
        ChromiumPlugin::instance_ = nullptr;
    }
    
    blog(LOG_INFO, "[Chromium Plugin] Plugin unloaded");
}

const char* obs_module_description(void) {
    return PLUGIN_DESCRIPTION;
}

const char* obs_module_name(void) {
    return PLUGIN_NAME;
}

// ChromiumPlugin implementation
ChromiumPlugin::ChromiumPlugin() : initialized_(false) {
}

ChromiumPlugin::~ChromiumPlugin() {
    if (initialized_) {
        Shutdown();
    }
}

ChromiumPlugin* ChromiumPlugin::GetInstance() {
    if (!instance_) {
        instance_ = new ChromiumPlugin();
    }
    return instance_;
}

bool ChromiumPlugin::Initialize() {
    if (initialized_) {
        return true;
    }
    
    blog(LOG_INFO, "[Chromium Plugin] Initializing CEF framework");
    
    try {
        InitializeCEF();
        initialized_ = true;
        blog(LOG_INFO, "[Chromium Plugin] CEF framework initialized successfully");
        return true;
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[Chromium Plugin] Failed to initialize CEF: %s", e.what());
        return false;
    }
}

void ChromiumPlugin::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    blog(LOG_INFO, "[Chromium Plugin] Shutting down CEF framework");
    
    try {
        ShutdownCEF();
        initialized_ = false;
        blog(LOG_INFO, "[Chromium Plugin] CEF framework shut down successfully");
    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[Chromium Plugin] Error during CEF shutdown: %s", e.what());
    }
}

void ChromiumPlugin::InitializeCEF() {
    if (!CEFManager::Initialize()) {
        throw std::runtime_error("Failed to initialize CEF framework");
    }
}

void ChromiumPlugin::ShutdownCEF() {
    CEFManager::Shutdown();
}

// ChromiumSource implementation
ChromiumSource::ChromiumSource() 
    : source(nullptr)
    , width(DEFAULT_WIDTH)
    , height(DEFAULT_HEIGHT)
    , force_continuous_playback(DEFAULT_FORCE_CONTINUOUS)
    , volume(DEFAULT_VOLUME)
    , muted(false)
    , texture(nullptr)
    , audio_source(nullptr) {
    
    url = DEFAULT_URL;
    
    // Initialize mutex for thread-safe texture access
    if (pthread_mutex_init(&texture_mutex, nullptr) != 0) {
        blog(LOG_ERROR, "[Chromium Source] Failed to initialize texture mutex");
    }
}

ChromiumSource::~ChromiumSource() {
    // Cleanup texture
    if (texture) {
        obs_enter_graphics();
        gs_texture_destroy(texture);
        obs_leave_graphics();
        texture = nullptr;
    }
    
    // Cleanup mutex
    pthread_mutex_destroy(&texture_mutex);
    
    // Browser and audio cleanup will be handled by their destructors
}