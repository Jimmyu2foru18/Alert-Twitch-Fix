#include "cef_audio.h"
#include "chromium_source.h"
#include <obs-module.h>
#include <media-io/audio-resampler.h>
#include <util/circlebuf.h>
#include <util/platform.h>
#include <algorithm>
#include <cstring>

// CEFAudioHandler implementation
CEFAudioHandler::CEFAudioHandler(ChromiumSource* source)
    : source_(source)
    , stream_active_(false)
    , volume_(1.0f)
    , muted_(false)
    , resampler_(nullptr) {
    
    // Initialize audio buffer
    circlebuf_init(&audio_buffer_);
    
    // Initialize buffer mutex
    if (pthread_mutex_init(&buffer_mutex_, nullptr) != 0) {
        blog(LOG_ERROR, "[CEF Audio] Failed to initialize buffer mutex");
    }
    
    // Set default output parameters for OBS
    output_params_.sample_rate = 48000;
    output_params_.channels = 2;
    output_params_.format = AUDIO_FORMAT_FLOAT_PLANAR;
    output_params_.frames_per_buffer = 1024;
}

CEFAudioHandler::~CEFAudioHandler() {
    CleanupResampler();
    
    // Cleanup audio buffer
    pthread_mutex_lock(&buffer_mutex_);
    circlebuf_free(&audio_buffer_);
    pthread_mutex_unlock(&buffer_mutex_);
    
    // Destroy mutex
    pthread_mutex_destroy(&buffer_mutex_);
}

bool CEFAudioHandler::GetAudioParameters(CefRefPtr<CefBrowser> browser,
                                        CefAudioParameters& params) {
    // Request high-quality audio parameters
    params.channel_layout = CEF_CHANNEL_LAYOUT_STEREO;
    params.sample_rate = 48000;
    params.frames_per_buffer = 1024;
    
    blog(LOG_INFO, "[CEF Audio] Requested audio parameters: %d Hz, stereo, %d frames",
         params.sample_rate, params.frames_per_buffer);
    
    return true;
}

void CEFAudioHandler::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                                          const CefAudioParameters& params,
                                          int channels) {
    blog(LOG_INFO, "[CEF Audio] Audio stream started: %d Hz, %d channels, %d frames",
         params.sample_rate, channels, params.frames_per_buffer);
    
    // Store input parameters
    input_params_.sample_rate = params.sample_rate;
    input_params_.channels = channels;
    input_params_.format = AUDIO_FORMAT_FLOAT;
    input_params_.frames_per_buffer = params.frames_per_buffer;
    
    // Initialize resampler if needed
    InitializeResampler();
    
    stream_active_ = true;
}

void CEFAudioHandler::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                                         const float** data,
                                         int frames,
                                         int64_t pts) {
    if (!stream_active_ || !data || frames <= 0) {
        return;
    }
    
    // Process the audio data
    ProcessAudioData(data, frames, pts);
}

void CEFAudioHandler::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {
    blog(LOG_INFO, "[CEF Audio] Audio stream stopped");
    stream_active_ = false;
    
    // Clear audio buffer
    pthread_mutex_lock(&buffer_mutex_);
    circlebuf_free(&audio_buffer_);
    circlebuf_init(&audio_buffer_);
    pthread_mutex_unlock(&buffer_mutex_);
}

void CEFAudioHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser,
                                        const CefString& message) {
    blog(LOG_WARNING, "[CEF Audio] Audio stream error: %s", message.ToString().c_str());
    stream_active_ = false;
}

void CEFAudioHandler::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
}

float CEFAudioHandler::GetVolume() const {
    return volume_;
}

void CEFAudioHandler::SetMuted(bool muted) {
    muted_ = muted;
}

bool CEFAudioHandler::IsMuted() const {
    return muted_;
}

bool CEFAudioHandler::IsStreamActive() const {
    return stream_active_;
}

void CEFAudioHandler::InitializeResampler() {
    CleanupResampler();
    
    // Check if resampling is needed
    if (input_params_.sample_rate == output_params_.sample_rate &&
        input_params_.channels == output_params_.channels) {
        // No resampling needed
        return;
    }
    
    // Create resampler
    struct resample_info src_info = {};
    src_info.samples_per_sec = input_params_.sample_rate;
    src_info.format = AUDIO_FORMAT_FLOAT;
    src_info.speakers = input_params_.channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
    
    struct resample_info dst_info = {};
    dst_info.samples_per_sec = output_params_.sample_rate;
    dst_info.format = output_params_.format;
    dst_info.speakers = output_params_.channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
    
    resampler_ = audio_resampler_create(&dst_info, &src_info);
    
    if (resampler_) {
        blog(LOG_INFO, "[CEF Audio] Created resampler: %d Hz -> %d Hz, %d -> %d channels",
             input_params_.sample_rate, output_params_.sample_rate,
             input_params_.channels, output_params_.channels);
    } else {
        blog(LOG_ERROR, "[CEF Audio] Failed to create audio resampler");
    }
}

void CEFAudioHandler::CleanupResampler() {
    if (resampler_) {
        audio_resampler_destroy(resampler_);
        resampler_ = nullptr;
    }
}

void CEFAudioHandler::ProcessAudioData(const float** data, int frames, int64_t pts) {
    if (muted_ || volume_ <= 0.0f) {
        return;
    }
    
    // Convert and buffer the audio data
    ConvertAndBuffer(data, frames);
}

void CEFAudioHandler::ConvertAndBuffer(const float** input_data, int frames) {
    if (!input_data || frames <= 0) {
        return;
    }
    
    pthread_mutex_lock(&buffer_mutex_);
    
    if (resampler_) {
        // Resample the audio
        uint8_t* output_data[MAX_AV_PLANES] = {0};
        uint32_t output_frames = 0;
        uint64_t ts_offset = 0;
        
        bool success = audio_resampler_resample(resampler_,
                                               output_data,
                                               &output_frames,
                                               &ts_offset,
                                               (const uint8_t**)input_data,
                                               frames);
        
        if (success && output_frames > 0) {
            // Apply volume adjustment
            float** float_output = (float**)output_data;
            AudioUtils::ApplyVolume(float_output, output_frames, output_params_.channels, volume_);
            
            // Buffer the resampled data
            size_t data_size = AudioUtils::GetAudioDataSize(output_frames, output_params_.channels, output_params_.format);
            circlebuf_push_back(&audio_buffer_, output_data[0], data_size);
        }
    } else {
        // No resampling needed, just apply volume and buffer
        size_t data_size = frames * input_params_.channels * sizeof(float);
        
        if (volume_ != 1.0f) {
            // Create a copy and apply volume
            std::vector<float> volume_adjusted(frames * input_params_.channels);
            
            for (int ch = 0; ch < input_params_.channels; ++ch) {
                for (int i = 0; i < frames; ++i) {
                    volume_adjusted[i * input_params_.channels + ch] = input_data[ch][i] * volume_;
                }
            }
            
            circlebuf_push_back(&audio_buffer_, volume_adjusted.data(), data_size);
        } else {
            // Convert planar to interleaved and buffer
            std::vector<float> interleaved(frames * input_params_.channels);
            AudioUtils::PlanarToInterleaved((float**)input_data, interleaved.data(), frames, input_params_.channels);
            circlebuf_push_back(&audio_buffer_, interleaved.data(), data_size);
        }
    }
    
    pthread_mutex_unlock(&buffer_mutex_);
}

// CEFAudio implementation
CEFAudio::CEFAudio(ChromiumSource* source)
    : source_(source)
    , audio_source_(nullptr)
    , initialized_(false) {
    
    audio_handler_ = new CEFAudioHandler(source);
}

CEFAudio::~CEFAudio() {
    Shutdown();
}

bool CEFAudio::Initialize() {
    if (initialized_) {
        return true;
    }
    
    CreateAudioSource();
    initialized_ = true;
    
    blog(LOG_INFO, "[CEF Audio] Audio system initialized");
    return true;
}

void CEFAudio::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    DestroyAudioSource();
    initialized_ = false;
    
    blog(LOG_INFO, "[CEF Audio] Audio system shut down");
}

CefRefPtr<CEFAudioHandler> CEFAudio::GetAudioHandler() {
    return audio_handler_;
}

void CEFAudio::SetVolume(float volume) {
    if (audio_handler_) {
        audio_handler_->SetVolume(volume);
    }
}

float CEFAudio::GetVolume() const {
    if (audio_handler_) {
        return audio_handler_->GetVolume();
    }
    return 1.0f;
}

void CEFAudio::SetMuted(bool muted) {
    if (audio_handler_) {
        audio_handler_->SetMuted(muted);
    }
}

bool CEFAudio::IsMuted() const {
    if (audio_handler_) {
        return audio_handler_->IsMuted();
    }
    return false;
}

bool CEFAudio::IsAudioActive() const {
    if (audio_handler_) {
        return audio_handler_->IsStreamActive();
    }
    return false;
}

obs_source_t* CEFAudio::GetAudioSource() const {
    return audio_source_;
}

void CEFAudio::CreateAudioSource() {
    if (audio_source_) {
        return;
    }
    
    // Create audio source settings
    obs_data_t* settings = obs_data_create();
    obs_data_set_string(settings, "name", "Chromium Audio");
    
    // Register audio source type if not already registered
    static bool audio_source_registered = false;
    if (!audio_source_registered) {
        struct obs_source_info audio_source_info = {};
        audio_source_info.id = "chromium_audio_source";
        audio_source_info.type = OBS_SOURCE_TYPE_INPUT;
        audio_source_info.output_flags = OBS_SOURCE_AUDIO;
        audio_source_info.get_name = [](void*) -> const char* { return "Chromium Audio Source"; };
        audio_source_info.create = audio_source_create;
        audio_source_info.destroy = audio_source_destroy;
        audio_source_info.update = audio_source_update;
        audio_source_info.get_properties = audio_source_get_properties;
        audio_source_info.get_defaults = audio_source_get_defaults;
        
        obs_register_source(&audio_source_info);
        audio_source_registered = true;
    }
    
    // Create the audio source
    audio_source_ = obs_source_create("chromium_audio_source", "Chromium Audio", settings, nullptr);
    
    obs_data_release(settings);
    
    if (audio_source_) {
        blog(LOG_INFO, "[CEF Audio] Audio source created successfully");
    } else {
        blog(LOG_ERROR, "[CEF Audio] Failed to create audio source");
    }
}

void CEFAudio::DestroyAudioSource() {
    if (audio_source_) {
        obs_source_release(audio_source_);
        audio_source_ = nullptr;
    }
}

// Audio source callbacks
void* CEFAudio::audio_source_create(obs_data_t* settings, obs_source_t* source) {
    // Return a dummy pointer since we manage audio through CEFAudio class
    return (void*)1;
}

void CEFAudio::audio_source_destroy(void* data) {
    // Nothing to destroy here
}

void CEFAudio::audio_source_update(void* data, obs_data_t* settings) {
    // Nothing to update here
}

obs_properties_t* CEFAudio::audio_source_get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    obs_properties_add_text(props, "info", "Chromium Audio Source", OBS_TEXT_INFO);
    return props;
}

void CEFAudio::audio_source_get_defaults(obs_data_t* settings) {
    // No defaults needed
}

// AudioUtils implementation
namespace AudioUtils {

enum audio_format CEFToOBSAudioFormat(int sample_type) {
    // CEF typically provides float samples
    return AUDIO_FORMAT_FLOAT;
}

size_t GetAudioDataSize(uint32_t frames, uint32_t channels, enum audio_format format) {
    size_t bytes_per_sample = 0;
    
    switch (format) {
        case AUDIO_FORMAT_U8BIT:
        case AUDIO_FORMAT_U8BIT_PLANAR:
            bytes_per_sample = 1;
            break;
        case AUDIO_FORMAT_16BIT:
        case AUDIO_FORMAT_16BIT_PLANAR:
            bytes_per_sample = 2;
            break;
        case AUDIO_FORMAT_32BIT:
        case AUDIO_FORMAT_32BIT_PLANAR:
        case AUDIO_FORMAT_FLOAT:
        case AUDIO_FORMAT_FLOAT_PLANAR:
            bytes_per_sample = 4;
            break;
        default:
            bytes_per_sample = 4;
            break;
    }
    
    return frames * channels * bytes_per_sample;
}

void ApplyVolume(float** audio_data, uint32_t frames, uint32_t channels, float volume) {
    if (!audio_data || volume == 1.0f) {
        return;
    }
    
    for (uint32_t ch = 0; ch < channels; ++ch) {
        if (audio_data[ch]) {
            for (uint32_t i = 0; i < frames; ++i) {
                audio_data[ch][i] *= volume;
            }
        }
    }
}

void InterleavedToPlanar(const float* interleaved, float** planar,
                        uint32_t frames, uint32_t channels) {
    if (!interleaved || !planar) {
        return;
    }
    
    for (uint32_t ch = 0; ch < channels; ++ch) {
        if (planar[ch]) {
            for (uint32_t i = 0; i < frames; ++i) {
                planar[ch][i] = interleaved[i * channels + ch];
            }
        }
    }
}

void PlanarToInterleaved(float** planar, float* interleaved,
                        uint32_t frames, uint32_t channels) {
    if (!planar || !interleaved) {
        return;
    }
    
    for (uint32_t i = 0; i < frames; ++i) {
        for (uint32_t ch = 0; ch < channels; ++ch) {
            if (planar[ch]) {
                interleaved[i * channels + ch] = planar[ch][i];
            } else {
                interleaved[i * channels + ch] = 0.0f;
            }
        }
    }
}

} // namespace AudioUtils