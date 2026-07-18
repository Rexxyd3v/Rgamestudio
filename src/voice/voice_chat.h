#ifndef VOICE_CHAT_H
#define VOICE_CHAT_H

#include <opus/opus.h>
#include <miniaudio/miniaudio.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <map>
#include "../network/packets.h"

// Voice chat configuration
constexpr int SAMPLE_RATE = 48000;      // Hz
constexpr int CHANNELS = 1;             // Mono
constexpr int FRAME_SIZE_MS = 20;       // ms
constexpr int FRAME_SIZE = (SAMPLE_RATE * FRAME_SIZE_MS) / 1000; // Samples per frame
constexpr int BITRATE = 20000;          // bps
constexpr int MAX_CLIENTS = 32;         // Maximum number of concurrent voice streams
// Keep PacketVoiceData::MAX_OPUS_FRAME_SIZE from packets.h to avoid macro conflicts.


class VoiceChat {
public:
    VoiceChat();
    ~VoiceChat();

    // Returns true if local player is currently transmitting (push-to-talk held).
    bool isLocalSpeaking() const;

    // Returns true if we recently received voice from userID.
    bool isUserSpeaking(uint32_t userID) const;


    // Initialize audio devices and codecs
    bool initialize();
    void shutdown();

    // Start/stop voice transmission (push-to-talk)
    void setTransmitEnabled(bool enabled);
    bool isTransmitEnabled() const { return m_transmitEnabled.load(); }

    // Process audio capture (call from main loop)
    void captureAudio();

    // Process incoming voice packet
    void processVoicePacket(const PacketVoiceData* packet, uint32_t senderID);

    // Update audio playback (call from main loop)
    void updatePlayback(float deltaTime);

    // Set volume for a specific user (0.0 to 1.0)
    void setUserVolume(uint32_t userID, float volume);

    // Mute/unmute a specific user
    void setUserMuted(uint32_t userID, bool muted);

private:
    // Audio capture callback for miniaudio
    static void captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // Audio playback callback for miniaudio
    static void playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // Encode PCM to Opus
    std::vector<uint8_t> encodeOpus(const int16_t* pcmData, size_t frameCount);

    // Mix audio from multiple sources
    void mixAudio(float* output, const std::vector<float*>& inputs, size_t frameCount, size_t channelCount);

    // Member variables
    ma_device m_captureDevice;
    ma_device m_playbackDevice;
    OpusEncoder* m_encoder;
    std::map<uint32_t, OpusDecoder*> m_decoders; // One decoder per remote user

    std::atomic<bool> m_transmitEnabled;
    std::atomic<bool> m_initialized;

    // Audio buffers
    std::vector<int16_t> m_captureBuffer;
    std::vector<float>   m_mixBuffer;

    // Jitter buffer for incoming audio
    struct VoicePacket {
        uint32_t senderID;
        uint16_t sequenceNumber;
        std::vector<uint8_t> opusData;
        uint64_t timestamp;
    };

    std::queue<VoicePacket> m_jitterBuffer;
    std::mutex              m_jitterMutex;
    std::condition_variable m_jitterCondition;

    // Per-user volume and mute state
    std::map<uint32_t, float>   m_userVolumes;
    std::map<uint32_t, bool>    m_userMuted;

    // Sequence tracking for each user
    std::map<uint32_t, uint16_t> m_lastSequenceNum;

    // Timestamp for jitter buffer
    uint64_t m_baseTimestamp;

    // UI indicator timers
    std::atomic<uint64_t> m_localSpeakUntilMs;
    std::map<uint32_t, uint64_t> m_lastHeardMs;
};

#endif // VOICE_CHAT_H

