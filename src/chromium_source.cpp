#include "chromium_source.h"
#include "cef_browser.h"
#include "cef_audio.h"
#include <obs-module.h>
#include <obs-properties.h>
#include <graphics/graphics.h>
#include <util/platform.h>
#include <algorithm>
#include <cmath>

// OBS source callbacks implementation
void* chromium_source_create(obs_data_t* settings, obs_source_t* source) {
    blog(LOG_INFO, "[Chromium Source] Creating new source instance");
    
    ChromiumSourceImpl* impl = new ChromiumSourceImpl(source);
    if (!impl->Initialize(settings)) {
        blog(LOG_ERROR, "[Chromium Source] Failed to initialize source");
        delete impl;
        return nullptr;
    }
    
    return impl;
}

void chromium_source_destroy(void* data) {
    if (!data) return;
    
    blog(LOG_INFO, "[Chromium Source] Destroying source instance");
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    delete impl;
}

void chromium_source_update(void* data, obs_data_t* settings) {
    if (!data) return;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    impl->Update(settings);
}

void chromium_source_video_tick(void* data, float seconds) {
    if (!data) return;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    impl->VideoTick(seconds);
}

void chromium_source_video_render(void* data, gs_effect_t* effect) {
    if (!data) return;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    impl->VideoRender(effect);
}

uint32_t chromium_source_get_width(void* data) {
    if (!data) return 0;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    return impl->GetWidth();
}

uint32_t chromium_source_get_height(void* data) {
    if (!data) return 0;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    return impl->GetHeight();
}

obs_properties_t* chromium_source_get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    
    // URL input
    obs_property_t* url_prop = obs_properties_add_text(props, PROP_URL, TEXT_URL, OBS_TEXT_DEFAULT);
    obs_property_set_long_description(url_prop, TEXT_URL_TOOLTIP);
    obs_property_set_modified_callback(url_prop, ChromiumSourceProperties::url_modified);
    
    // Size preset dropdown
    obs_property_t* preset_prop = obs_properties_add_list(props, PROP_SIZE_PRESET, TEXT_SIZE_PRESET, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_set_long_description(preset_prop, TEXT_SIZE_PRESET_TOOLTIP);
    
    for (size_t i = 0; i < NUM_SIZE_PRESETS; ++i) {
        obs_property_list_add_int(preset_prop, SIZE_PRESETS[i].name, i);
    }
    obs_property_set_modified_callback(preset_prop, ChromiumSourceProperties::size_preset_modified);
    
    // Custom size toggle
    obs_property_t* custom_size_prop = obs_properties_add_bool(props, PROP_CUSTOM_SIZE, TEXT_CUSTOM_SIZE);
    obs_property_set_long_description(custom_size_prop, TEXT_CUSTOM_SIZE_TOOLTIP);
    obs_property_set_modified_callback(custom_size_prop, ChromiumSourceProperties::custom_size_modified);
    
    // Width and height (initially hidden)
    obs_property_t* width_prop = obs_properties_add_int_slider(props, PROP_WIDTH, TEXT_WIDTH, MIN_WIDTH, MAX_WIDTH, 1);
    obs_property_set_long_description(width_prop, TEXT_WIDTH_TOOLTIP);
    
    obs_property_t* height_prop = obs_properties_add_int_slider(props, PROP_HEIGHT, TEXT_HEIGHT, MIN_HEIGHT, MAX_HEIGHT, 1);
    obs_property_set_long_description(height_prop, TEXT_HEIGHT_TOOLTIP);
    
    // Force continuous playback
    obs_property_t* continuous_prop = obs_properties_add_bool(props, PROP_FORCE_CONTINUOUS, TEXT_FORCE_CONTINUOUS);
    obs_property_set_long_description(continuous_prop, TEXT_FORCE_CONTINUOUS_TOOLTIP);
    
    // Volume slider
    obs_property_t* volume_prop = obs_properties_add_float_slider(props, PROP_VOLUME, TEXT_VOLUME, 0.0, 1.0, 0.01);
    obs_property_set_long_description(volume_prop, TEXT_VOLUME_TOOLTIP);
    
    // Mute checkbox
    obs_property_t* muted_prop = obs_properties_add_bool(props, PROP_MUTED, TEXT_MUTED);
    obs_property_set_long_description(muted_prop, TEXT_MUTED_TOOLTIP);
    
    // Reload button
    obs_property_t* reload_prop = obs_properties_add_button(props, PROP_RELOAD_BUTTON, TEXT_RELOAD_BUTTON, ChromiumSourceProperties::reload_button_clicked);
    obs_property_set_long_description(reload_prop, TEXT_RELOAD_BUTTON_TOOLTIP);
    
    // Advanced settings group
    obs_property_t* advanced_group = obs_properties_create_group(props, PROP_ADVANCED_GROUP, TEXT_ADVANCED_GROUP, OBS_GROUP_NORMAL);
    
    // Auto reload
    obs_property_t* auto_reload_prop = obs_properties_add_bool(advanced_group, PROP_AUTO_RELOAD, TEXT_AUTO_RELOAD);
    obs_property_set_long_description(auto_reload_prop, TEXT_AUTO_RELOAD_TOOLTIP);
    obs_property_set_modified_callback(auto_reload_prop, ChromiumSourceProperties::auto_reload_modified);
    
    // Reload interval
    obs_property_t* interval_prop = obs_properties_add_int_slider(advanced_group, PROP_RELOAD_INTERVAL, TEXT_RELOAD_INTERVAL, MIN_RELOAD_INTERVAL, MAX_RELOAD_INTERVAL, 1);
    obs_property_set_long_description(interval_prop, TEXT_RELOAD_INTERVAL_TOOLTIP);
    
    return props;
}

void chromium_source_get_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, PROP_URL, DEFAULT_URL);
    obs_data_set_default_int(settings, PROP_WIDTH, DEFAULT_WIDTH);
    obs_data_set_default_int(settings, PROP_HEIGHT, DEFAULT_HEIGHT);
    obs_data_set_default_int(settings, PROP_SIZE_PRESET, DEFAULT_SIZE_PRESET);
    obs_data_set_default_bool(settings, PROP_CUSTOM_SIZE, DEFAULT_CUSTOM_SIZE);
    obs_data_set_default_bool(settings, PROP_FORCE_CONTINUOUS, DEFAULT_FORCE_CONTINUOUS);
    obs_data_set_default_double(settings, PROP_VOLUME, DEFAULT_VOLUME);
    obs_data_set_default_bool(settings, PROP_MUTED, false);
    obs_data_set_default_bool(settings, PROP_AUTO_RELOAD, DEFAULT_AUTO_RELOAD);
    obs_data_set_default_int(settings, PROP_RELOAD_INTERVAL, DEFAULT_RELOAD_INTERVAL);
}

void chromium_source_enum_active_sources(void* data, obs_source_enum_proc_t enum_callback, void* param) {
    if (!data) return;
    
    ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
    impl->EnumActiveSources(enum_callback, param);
}

// ChromiumSourceImpl implementation
ChromiumSourceImpl::ChromiumSourceImpl(obs_source_t* source)
    : obs_source_(source)
    , width_(DEFAULT_WIDTH)
    , height_(DEFAULT_HEIGHT)
    , force_continuous_playback_(DEFAULT_FORCE_CONTINUOUS)
    , volume_(DEFAULT_VOLUME)
    , muted_(false)
    , auto_reload_(DEFAULT_AUTO_RELOAD)
    , reload_interval_(DEFAULT_RELOAD_INTERVAL)
    , texture_(nullptr)
    , last_reload_time_(0.0f) {
    
    url_ = DEFAULT_URL;
    
    // Initialize texture mutex
    if (pthread_mutex_init(&texture_mutex_, nullptr) != 0) {
        blog(LOG_ERROR, "[Chromium Source] Failed to initialize texture mutex");
    }
}

ChromiumSourceImpl::~ChromiumSourceImpl() {
    DestroyBrowser();
    
    // Cleanup texture
    if (texture_) {
        obs_enter_graphics();
        gs_texture_destroy(texture_);
        obs_leave_graphics();
        texture_ = nullptr;
    }
    
    // Cleanup mutex
    pthread_mutex_destroy(&texture_mutex_);
}

bool ChromiumSourceImpl::Initialize(obs_data_t* settings) {
    if (!CEFManager::IsInitialized()) {
        blog(LOG_ERROR, "[Chromium Source] Cannot initialize: CEF not ready");
        return false;
    }
    
    LoadSettings(settings);
    CreateBrowser();
    
    blog(LOG_INFO, "[Chromium Source] Source initialized successfully");
    return true;
}

void ChromiumSourceImpl::Update(obs_data_t* settings) {
    std::string old_url = url_;
    int old_width = width_;
    int old_height = height_;
    
    LoadSettings(settings);
    
    // Update audio settings
    if (audio_) {
        audio_->SetVolume(volume_);
        audio_->SetMuted(muted_);
    }
    
    // Check if browser needs to be recreated or updated
    if (url_ != old_url) {
        if (browser_ && browser_->IsValid()) {
            browser_->LoadURL(url_);
        } else {
            CreateBrowser();
        }
    }
    
    // Update browser size if changed
    if ((width_ != old_width || height_ != old_height) && browser_) {
        UpdateBrowserSize();
    }
}

void ChromiumSourceImpl::VideoTick(float seconds) {
    // Handle auto reload
    if (auto_reload_ && reload_interval_ > 0) {
        last_reload_time_ += seconds;
        if (last_reload_time_ >= reload_interval_) {
            ReloadBrowser();
            last_reload_time_ = 0.0f;
        }
    }
    
    // Force browser invalidation for continuous playback
    if (force_continuous_playback_ && browser_) {
        browser_->Invalidate();
    }
}

void ChromiumSourceImpl::VideoRender(gs_effect_t* effect) {
    pthread_mutex_lock(&texture_mutex_);
    
    if (texture_) {
        gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), texture_);
        gs_draw_sprite(texture_, 0, width_, height_);
    }
    
    pthread_mutex_unlock(&texture_mutex_);
}

uint32_t ChromiumSourceImpl::GetWidth() const {
    return width_;
}

uint32_t ChromiumSourceImpl::GetHeight() const {
    return height_;
}

void ChromiumSourceImpl::EnumActiveSources(obs_source_enum_proc_t enum_callback, void* param) {
    if (audio_ && audio_->GetAudioSource()) {
        enum_callback(obs_source_, audio_->GetAudioSource(), param);
    }
}

obs_source_t* ChromiumSourceImpl::GetSource() const {
    return obs_source_;
}

void ChromiumSourceImpl::LoadSettings(obs_data_t* settings) {
    // Load URL
    const char* url = obs_data_get_string(settings, PROP_URL);
    url_ = url ? url : DEFAULT_URL;
    
    // Load size settings
    bool custom_size = obs_data_get_bool(settings, PROP_CUSTOM_SIZE);
    if (custom_size) {
        width_ = (int)obs_data_get_int(settings, PROP_WIDTH);
        height_ = (int)obs_data_get_int(settings, PROP_HEIGHT);
    } else {
        int preset_index = (int)obs_data_get_int(settings, PROP_SIZE_PRESET);
        if (preset_index >= 0 && preset_index < NUM_SIZE_PRESETS) {
            const SizePreset& preset = SIZE_PRESETS[preset_index];
            if (preset.width > 0 && preset.height > 0) {
                width_ = preset.width;
                height_ = preset.height;
            }
        }
    }
    
    // Clamp dimensions
    width_ = std::clamp(width_, MIN_WIDTH, MAX_WIDTH);
    height_ = std::clamp(height_, MIN_HEIGHT, MAX_HEIGHT);
    
    // Load other settings
    force_continuous_playback_ = obs_data_get_bool(settings, PROP_FORCE_CONTINUOUS);
    volume_ = (float)obs_data_get_double(settings, PROP_VOLUME);
    muted_ = obs_data_get_bool(settings, PROP_MUTED);
    auto_reload_ = obs_data_get_bool(settings, PROP_AUTO_RELOAD);
    reload_interval_ = (int)obs_data_get_int(settings, PROP_RELOAD_INTERVAL);
    
    // Clamp values
    volume_ = std::clamp(volume_, 0.0f, 1.0f);
    reload_interval_ = std::clamp(reload_interval_, MIN_RELOAD_INTERVAL, MAX_RELOAD_INTERVAL);
}

void ChromiumSourceImpl::CreateBrowser() {
    DestroyBrowser();
    
    if (!CEFManager::IsInitialized()) {
        blog(LOG_ERROR, "[Chromium Source] Cannot create browser: CEF not initialized");
        return;
    }
    
    // Create audio system
    audio_ = std::make_unique<CEFAudio>(nullptr); // Pass nullptr since we're managing it here
    if (!audio_->Initialize()) {
        blog(LOG_ERROR, "[Chromium Source] Failed to initialize audio system");
        audio_.reset();
    }
    
    // Create browser
    browser_ = std::make_unique<CEFBrowser>(nullptr); // Pass nullptr since we're managing it here
    if (!browser_->Initialize(url_, width_, height_)) {
        blog(LOG_ERROR, "[Chromium Source] Failed to initialize browser");
        browser_.reset();
        return;
    }
    
    blog(LOG_INFO, "[Chromium Source] Browser created successfully for URL: %s", url_.c_str());
}

void ChromiumSourceImpl::DestroyBrowser() {
    if (browser_) {
        browser_->Close();
        browser_.reset();
    }
    
    if (audio_) {
        audio_->Shutdown();
        audio_.reset();
    }
}

void ChromiumSourceImpl::UpdateBrowserSize() {
    if (browser_ && browser_->IsValid()) {
        browser_->Resize(width_, height_);
        blog(LOG_INFO, "[Chromium Source] Browser resized to %dx%d", width_, height_);
    }
}

void ChromiumSourceImpl::ReloadBrowser() {
    if (browser_ && browser_->IsValid()) {
        browser_->Reload();
        blog(LOG_INFO, "[Chromium Source] Browser reloaded");
    }
}

// Property modification callbacks
namespace ChromiumSourceProperties {

bool url_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings) {
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    UNUSED_PARAMETER(settings);
    return true;
}

bool reload_button_clicked(obs_properties_t* props, obs_property_t* property, void* data) {
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    
    if (data) {
        ChromiumSourceImpl* impl = static_cast<ChromiumSourceImpl*>(data);
        impl->ReloadBrowser();
    }
    
    return false; // Don't refresh properties
}

bool size_preset_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings) {
    UNUSED_PARAMETER(property);
    
    int preset_index = (int)obs_data_get_int(settings, PROP_SIZE_PRESET);
    bool is_custom = (preset_index == 0); // First preset is "Custom"
    
    // Update custom size checkbox
    obs_data_set_bool(settings, PROP_CUSTOM_SIZE, is_custom);
    
    // Show/hide width and height controls
    obs_property_t* width_prop = obs_properties_get(props, PROP_WIDTH);
    obs_property_t* height_prop = obs_properties_get(props, PROP_HEIGHT);
    obs_property_t* custom_size_prop = obs_properties_get(props, PROP_CUSTOM_SIZE);
    
    obs_property_set_visible(width_prop, is_custom);
    obs_property_set_visible(height_prop, is_custom);
    obs_property_set_visible(custom_size_prop, is_custom);
    
    // Set preset dimensions if not custom
    if (!is_custom && preset_index > 0 && preset_index < NUM_SIZE_PRESETS) {
        const SizePreset& preset = SIZE_PRESETS[preset_index];
        obs_data_set_int(settings, PROP_WIDTH, preset.width);
        obs_data_set_int(settings, PROP_HEIGHT, preset.height);
    }
    
    return true;
}

bool custom_size_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings) {
    UNUSED_PARAMETER(property);
    
    bool custom_size = obs_data_get_bool(settings, PROP_CUSTOM_SIZE);
    
    // Show/hide width and height controls
    obs_property_t* width_prop = obs_properties_get(props, PROP_WIDTH);
    obs_property_t* height_prop = obs_properties_get(props, PROP_HEIGHT);
    
    obs_property_set_visible(width_prop, custom_size);
    obs_property_set_visible(height_prop, custom_size);
    
    // Update size preset to "Custom" if enabling custom size
    if (custom_size) {
        obs_data_set_int(settings, PROP_SIZE_PRESET, 0);
    }
    
    return true;
}

bool auto_reload_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings) {
    UNUSED_PARAMETER(property);
    
    bool auto_reload = obs_data_get_bool(settings, PROP_AUTO_RELOAD);
    
    // Show/hide reload interval control
    obs_property_t* interval_prop = obs_properties_get(props, PROP_RELOAD_INTERVAL);
    obs_property_set_visible(interval_prop, auto_reload);
    
    return true;
}

} // namespace ChromiumSourceProperties