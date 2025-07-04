#pragma once

#include <include/cef_audio_handler.h>
#include <include/cef_browser.h>
#include <obs-module.h>
#include <media-io/audio-resampler.h>
#include <util/circlebuf.h>
#include <util/threading.h>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

// Forward declaration
struct ChromiumSource;

/**
 * Audio parameters structure for managing audio format conversion.
 */
struct AudioParams {
    uint32_t sample_rate;
    uint32_t channels;
    enum audio_format format;
    uint32_t frames_per_buffer;
    
    AudioParams() 
        : sample_rate(48000)
        , channels(2)
        , format(AUDIO_FORMAT_FLOAT_PLANAR)
        , frames_per_buffer(1024) {
    }
};

/**
 * CEF Audio Handler that captures audio output from the browser.
 * This class implements the CefAudioHandler interface to receive
 * audio data from CEF and route it to the OBS audio subsystem.
 */
class CEFAudioHandler : public CefAudioHandler {
public:
    explicit CEFAudioHandler(ChromiumSource* source);
    ~CEFAudioHandler();
    
    // CefAudioHandler methods
    bool GetAudioParameters(CefRefPtr<CefBrowser> browser,
                           CefAudioParameters& params) override;
    
    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser,
                             const CefAudioParameters& params,
                             int channels) override;
    
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser,
                            const float** data,
                            int frames,
                            int64_t pts) override;
    
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override;
    
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser,
                           const CefString& message) override;
    
    /**
     * Set the volume level (0.0 to 1.0).
     */
    void SetVolume(float volume);
    
    /**
     * Get the current volume level.
     */
    float GetVolume() const;
    
    /**
     * Set mute state.
     */
    void SetMuted(bool muted);
    
    /**
     * Get mute state.
     */
    bool IsMuted() const;
    
    /**
     * Check if audio stream is active.
     */
    bool IsStreamActive() const;
    
private:
    ChromiumSource* source_;
    
    // Audio state
    std::atomic<bool> stream_active_;
    std::atomic<float> volume_;
    std::atomic<bool> muted_;
    
    // Audio format information
    AudioParams input_params_;
    AudioParams output_params_;
    
    // Audio resampling
    audio_resampler_t* resampler_;
    
    // Audio buffering
    struct circlebuf audio_buffer_;
    pthread_mutex_t buffer_mutex_;
    
    // Audio conversion helpers
    void InitializeResampler();
    void CleanupResampler();
    void ProcessAudioData(const float** data, int frames, int64_t pts);
    void ConvertAndBuffer(const float** input_data, int frames);
    
    IMPLEMENT_REFCOUNTING(CEFAudioHandler);
};

/**
 * CEF Audio Manager class that handles the integration between
 * CEF audio output and OBS audio sources.
 */
class CEFAudio {
public:
    explicit CEFAudio(ChromiumSource* source);
    ~CEFAudio();
    
    /**
     * Initialize the audio system and create OBS audio source.
     */
    bool Initialize();
    
    /**
     * Shutdown the audio system and cleanup resources.
     */
    void Shutdown();
    
    /**
     * Get the CEF audio handler for browser integration.
     */
    CefRefPtr<CEFAudioHandler> GetAudioHandler();
    
    /**
     * Set the volume level (0.0 to 1.0).
     */
    void SetVolume(float volume);
    
    /**
     * Get the current volume level.
     */
    float GetVolume() const;
    
    /**
     * Set mute state.
     */
    void SetMuted(bool muted);
    
    /**
     * Get mute state.
     */
    bool IsMuted() const;
    
    /**
     * Check if audio is available and active.
     */
    bool IsAudioActive() const;
    
    /**
     * Get the OBS audio source for enumeration.
     */
    obs_source_t* GetAudioSource() const;
    
private:
    ChromiumSource* source_;
    CefRefPtr<CEFAudioHandler> audio_handler_;
    obs_source_t* audio_source_;
    bool initialized_;
    
    // Audio source callbacks
    static void* audio_source_create(obs_data_t* settings, obs_source_t* source);
    static void audio_source_destroy(void* data);
    static void audio_source_update(void* data, obs_data_t* settings);
    static obs_properties_t* audio_source_get_properties(void* data);
    static void audio_source_get_defaults(obs_data_t* settings);
    
    // Audio data callback
    static bool audio_callback(void* param, uint64_t start_ts_in, uint64_t end_ts_in,
                              uint64_t* out_ts, uint32_t mixers,
                              struct audio_output_data* mixes);
    
    // Helper methods
    void CreateAudioSource();
    void DestroyAudioSource();
};

/**
 * Audio utility functions for format conversion and processing.
 */
namespace AudioUtils {
    /**
     * Convert CEF audio parameters to OBS audio format.
     */
    enum audio_format CEFToOBSAudioFormat(int sample_type);
    
    /**
     * Calculate the size of audio data in bytes.
     */
    size_t GetAudioDataSize(uint32_t frames, uint32_t channels, enum audio_format format);
    
    /**
     * Apply volume adjustment to audio data.
     */
    void ApplyVolume(float** audio_data, uint32_t frames, uint32_t channels, float volume);
    
    /**
     * Convert interleaved audio to planar format.
     */
    void InterleavedToPlanar(const float* interleaved, float** planar, 
                            uint32_t frames, uint32_t channels);
    
    /**
     * Convert planar audio to interleaved format.
     */
    void PlanarToInterleaved(float** planar, float* interleaved,
                            uint32_t frames, uint32_t channels);
}