#ifndef SIMPLE_VOICE_CHAT_H
#define SIMPLE_VOICE_CHAT_H

#include <opus/opus.h>
#include <miniaudio/miniaudio.h>
#include <atomic>
#include <string>
#include "../network/packets.h"
#include "../entities/character.h"

// Simple voice chat with proximity-based hearing
class SimpleVoiceChat {
public:
    SimpleVoiceChat();
    ~SimpleVoiceChat();

    // Initialize audio devices and codecs
    bool initialize();
    void shutdown();

    // Update call - call every frame
    void update(float deltaTime, Character* localPlayer);


    void processVoicePacket(const PacketVoiceData* packet, uint32_t senderID);

    // Set volume for a specific user (0.0 to 1.0)
    void setUserVolume(uint32_t userID, float volume);

    // Mute/unmute a specific user
    void setUserMuted(uint32_t userID, bool muted);

    // Debug: returns true if local player is currently transmitting
    bool isLocalSpeaking() const { return m_isTransmitting.load(); }

    // Debug: returns true if we recently heard from userID
    bool isUserSpeaking(uint32_t userID) const;

private:
    // Audio capture callback for miniaudio
    static void captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // Audio playback callback for miniaudio
    static void playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    // Encode PCM to Opus
    std::vector<uint8_t> encodeOpus(const int16_t* pcmData, size_t frameCount);

    // Decode Opus to PCM for a specific user
    bool decodeAndQueueOpus(uint32_t senderID, const uint8_t* opusData, size_t opusSize);

    // Mix audio from all queued sources
    void mixAndPlayAudio();

    // Check if a player is within hearing range
    bool isWithinHearingRange(Character* listener, Character* talker, float maxDistance = 1000.0f);

    // Member variables
    ma_device m_captureDevice;
    ma_device m_playbackDevice;
    OpusEncoder* m_encoder;
    std::map<uint32_t, OpusDecoder*> m_decoders; // One decoder per user

    std::atomic<bool> m_isTransmitting;
    std::atomic<bool> m_initialized;

    // Audio buffers
    std::vector<int16_t> m_captureBuffer;
    std::vector<float>   m_mixBuffer;
    std::vector<int16_t> m_decodeBuffer;

    // Per-user audio queues (simple implementation)
    std::map<uint32_t, std::vector<int16_t>> m_audioQueues;
    std::map<uint32_t, uint64_t> m_lastHeardTime; // For debug

    // Per-user volume and mute state
    std::map<uint32_t, float>   m_userVolumes;
    std::map<uint32_t, bool>    m_userMuted;

    // Configuration
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CHANNELS = 1;
    static constexpr int FRAME_SIZE_MS = 20;
    static constexpr int FRAME_SIZE = (SAMPLE_RATE * FRAME_SIZE_MS) / 1000;
    static constexpr int BITRATE = 20000;
    static constexpr float HEARING_DISTANCE = 1000.0f; // Units in game world
    static constexpr uint64_t SPEAK_TIMEOUT_MS = 1000; // How long to show "speaking" indicator
};

#endif // SIMPLE_VOICE_CHAT_H