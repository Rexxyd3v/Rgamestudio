#include "simple_voice_chat.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include "../network/network_manager.h"
#include "../entities/character.h"

// Opus error checking macro
#define OPUS_CHECK(call) \
    do { \
        int err = (call); \
        if (err < 0) { \
            std::cerr << "Opus error: " << opus_strerror(err) << " (" << #call << ")" << std::endl; \
        } \
    } while (0)

// Static frame counter for voice packets
static uint16_t s_voiceSequenceCounter = 0;

SimpleVoiceChat::SimpleVoiceChat()
    : m_encoder(nullptr)
    , m_isTransmitting(false)
    , m_initialized(false)
{
    // Initialize buffers
    m_captureBuffer.resize(FRAME_SIZE);
    m_mixBuffer.resize(FRAME_SIZE * 2); // Stereo for processing
    m_decodeBuffer.resize(FRAME_SIZE);
}

SimpleVoiceChat::~SimpleVoiceChat() {
    shutdown();
}

bool SimpleVoiceChat::initialize() {
    if (m_initialized.load()) {
        return true;
    }

    // Initialize Opus encoder
    int opusError;
    m_encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &opusError);
    if (!m_encoder || opusError < 0) {
        std::cerr << "Failed to create Opus encoder: " << opus_strerror(opusError) << std::endl;
        return false;
    }

    // Set encoder parameters
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(BITRATE));
    opus_encoder_ctl(m_encoder, OPUS_SET_VBR(1)); // Variable bitrate
    opus_encoder_ctl(m_encoder, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(5));
    opus_encoder_ctl(m_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(m_encoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
    opus_encoder_ctl(m_encoder, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(m_encoder, OPUS_SET_PACKET_LOSS_PERC(0));

    // Initialize audio devices
    ma_device_config config;

    // Capture device (microphone)
    config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID  = nullptr; // Default device
    config.capture.format     = ma_format_s16; // 16-bit
    config.capture.channels   = CHANNELS;
    config.capture.shareMode  = ma_share_mode_shared;
    config.sampleRate         = SAMPLE_RATE;
config.dataCallback = captureCallback;
    config.pUserData          = this;

    if (ma_device_init(nullptr, &config, &m_captureDevice) != MA_SUCCESS) {
        std::cerr << "Failed to initialize capture device." << std::endl;
        opus_encoder_destroy(m_encoder);
        return false;
    }

    // Playback device (speakers)
    config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = nullptr; // Default device
    config.playback.format    = ma_format_f32; // 32-bit float for processing
    config.playback.channels  = CHANNELS;
    config.sampleRate         = SAMPLE_RATE;
config.dataCallback = playbackCallback;
    config.pUserData          = this;

    if (ma_device_init(nullptr, &config, &m_playbackDevice) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device." << std::endl;
        ma_device_uninit(&m_captureDevice);
        opus_encoder_destroy(m_encoder);
        return false;
    }

    // Start audio devices
    if (ma_device_start(&m_captureDevice) != MA_SUCCESS) {
        std::cerr << "Failed to start capture device." << std::endl;
        ma_device_uninit(&m_captureDevice);
        ma_device_uninit(&m_playbackDevice);
        opus_encoder_destroy(m_encoder);
        return false;
    }

    if (ma_device_start(&m_playbackDevice) != MA_SUCCESS) {
        std::cerr << "Failed to start playback device." << std::endl;
        ma_device_uninit(&m_captureDevice);
        ma_device_uninit(&m_playbackDevice);
        opus_encoder_destroy(m_encoder);
        return false;
    }

    m_initialized.store(true);
    std::cout << "Simple voice chat initialized successfully." << std::endl;
    return true;
}

void SimpleVoiceChat::shutdown() {
    if (!m_initialized.exchange(false)) {
        return;
    }

    // Stop audio devices
    ma_device_stop(&m_captureDevice);
    ma_device_stop(&m_playbackDevice);

    // Uninitialize audio devices
    ma_device_uninit(&m_captureDevice);
    ma_device_uninit(&m_playbackDevice);

    // Destroy Opus encoder
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }

    // Destroy all decoders
    for (auto& pair : m_decoders) {
        if (pair.second) {
            opus_decoder_destroy(pair.second);
        }
    }
    m_decoders.clear();

    std::cout << "Simple voice chat shut down." << std::endl;
}

void SimpleVoiceChat::update(float deltaTime, Character* localPlayer) {
    if (!m_initialized.load() || !localPlayer) {
        return;
    }

    // Mix and play audio (this will play any queued audio)
    mixAndPlayAudio();
}

void SimpleVoiceChat::processVoicePacket(const PacketVoiceData* packet, uint32_t senderID) {
    if (!m_initialized.load() || !packet) {
        return;
    }

    // Check if the sender is within hearing range

    // We need to find the sender's character to check distance
    // For now, we'll assume all players are within range and let the game logic handle filtering
    // In a more complete implementation, we would maintain a list of all characters

    // Decode and queue the opus packet for playback
    decodeAndQueueOpus(senderID, packet->opusData, packet->frameSize);

    // Update last heard time for debug
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    m_lastHeardTime[senderID] = now;
}

bool SimpleVoiceChat::decodeAndQueueOpus(uint32_t senderID, const uint8_t* opusData, size_t opusSize) {
    // Get or create decoder for this user
    OpusDecoder* decoder = nullptr;
    auto it = m_decoders.find(senderID);
    if (it == m_decoders.end()) {
        int opusError;
        decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusError);
        if (!decoder || opusError < 0) {
            std::cerr << "Failed to create Opus decoder for user " << senderID << ": "
                      << opus_strerror(opusError) << std::endl;
            return false;
        }
        m_decoders[senderID] = decoder;
        m_userVolumes[senderID] = 1.0f;
        m_userMuted[senderID] = false;
    } else {
        decoder = it->second;
    }

    // Decode the opus packet
    int decodedFrames = opus_decode(
        decoder,
        opusData,
        static_cast<opus_int32>(opusSize),
        m_decodeBuffer.data(),
        FRAME_SIZE,
        0 // No PLC
    );

    if (decodedFrames <= 0) {
        std::cerr << "Failed to decode Opus packet from user " << senderID << ": "
                  << opus_strerror(decodedFrames) << std::endl;
        return false;
    }

    // Resize buffer to actual decoded size and queue for playback
    m_decodeBuffer.resize(decodedFrames);
    m_audioQueues[senderID].insert(
        m_audioQueues[senderID].end(),
        m_decodeBuffer.begin(),
        m_decodeBuffer.end()
    );

    return true;
}

void SimpleVoiceChat::mixAndPlayAudio() {
    // Clear mix buffer
    std::fill(m_mixBuffer.begin(), m_mixBuffer.end(), 0.0f);

    // Mix audio from all queued sources
    for (auto& queuePair : m_audioQueues) {
        uint32_t userID = queuePair.first;
        std::vector<int16_t>& audioQueue = queuePair.second;

        // Skip if muted
        if (m_userMuted[userID]) {
            audioQueue.clear();
            continue;
        }

        // Get volume for this user
        float volume = m_userVolumes.count(userID) ? m_userVolumes[userID] : 1.0f;

        // Process audio samples in chunks of FRAME_SIZE
        while (audioQueue.size() >= FRAME_SIZE) {
            // Take a frame from the queue
            std::vector<int16_t> frame(audioQueue.begin(), audioQueue.begin() + FRAME_SIZE);
            audioQueue.erase(audioQueue.begin(), audioQueue.begin() + FRAME_SIZE);

            // Convert to float and apply volume
            for (size_t i = 0; i < frame.size(); ++i) {
                // Convert from int16 to float (-1.0 to 1.0 range)
                float sample = static_cast<float>(frame[i]) / 32768.0f;
                m_mixBuffer[i] += sample * volume;
            }
        }

        // Clear any remaining partial frame (shouldn't happen in ideal case, but be safe)
        if (!audioQueue.empty()) {
            audioQueue.clear();
        }
    }

    // Clipping prevention
    for (float& sample : m_mixBuffer) {
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
    }
}

// Static callback for audio capture
void SimpleVoiceChat::captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    SimpleVoiceChat* self = static_cast<SimpleVoiceChat*>(pDevice->pUserData);
    if (!self || !self->m_initialized.load()) {
        return;
    }

    // Only process if transmission is enabled
    if (!self->m_isTransmitting.load()) {
        // Silence the output when not transmitting
        if (pOutput && frameCount > 0) {
            std::memset(pOutput, 0, frameCount * pDevice->capture.channels * ma_get_bytes_per_sample(pDevice->capture.format));
        }
        return;
    }

    // Copy input data to our buffer
    const int16_t* pcmInput = static_cast<const int16_t*>(pInput);
    size_t samplesToCopy = std::min(static_cast<size_t>(frameCount), self->m_captureBuffer.size());
    std::memcpy(self->m_captureBuffer.data(), pcmInput, samplesToCopy * sizeof(int16_t));

    // Encode and queue for transmission if we have a full frame
    if (self->m_encoder && self->m_captureBuffer.size() >= FRAME_SIZE) {
        std::vector<uint8_t> opusData = self->encodeOpus(self->m_captureBuffer.data(), FRAME_SIZE);

        if (!opusData.empty()) {
            // Create and send voice packet
            PacketVoiceData packet;
            packet.header.type = PacketType::VOICE_DATA;
            packet.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
            packet.sequenceNumber = ++s_voiceSequenceCounter;
            packet.frameSize = opusData.size();
            std::memcpy(packet.opusData, opusData.data(), opusData.size());

            // Send via network manager (unreliable for voice)
            size_t packetSize = sizeof(PacketHeader) + sizeof(packet.sequenceNumber) + sizeof(packet.frameSize) + packet.frameSize;
            NetworkManager::GetInstance().SendPacket(&packet, packetSize, false); // Unreliable
        }
    }
}

// Static callback for audio playback
void SimpleVoiceChat::playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    SimpleVoiceChat* self = static_cast<SimpleVoiceChat*>(pDevice->pUserData);
    if (!self || !self->m_initialized.load()) {
        return;
    }

    // Fill output with our mixed audio
    float* pOutputFloat = static_cast<float*>(pOutput);
    size_t samplesToWrite = std::min(static_cast<size_t>(frameCount) * pDevice->playback.channels, self->m_mixBuffer.size());

    for (size_t i = 0; i < samplesToWrite; ++i) {
        pOutputFloat[i] = self->m_mixBuffer[i];
    }

    // Zero out any remaining samples
    size_t totalSamples = frameCount * pDevice->playback.channels;
    if (totalSamples > samplesToWrite) {
        std::memset(pOutputFloat + samplesToWrite, 0, (totalSamples - samplesToWrite) * sizeof(float));
    }
}

std::vector<uint8_t> SimpleVoiceChat::encodeOpus(const int16_t* pcmData, size_t frameCount) {
    if (!m_encoder) {
        return {};
    }

    // Allocate output buffer (Opus max size is 1275 bytes per frame for safety)
    std::vector<uint8_t> opusData(4000);

    int encodedBytes = opus_encode(
        m_encoder,
        pcmData,
        static_cast<opus_int32>(frameCount),
        opusData.data(),
        static_cast<opus_int32>(opusData.size())
    );

    if (encodedBytes < 0) {
        std::cerr << "Opus encoding failed: " << opus_strerror(encodedBytes) << std::endl;
        return {};
    }

    opusData.resize(encodedBytes);
    return opusData;
}

bool SimpleVoiceChat::isWithinHearingRange(Character* listener, Character* talker, float maxDistance) {
    if (!listener || !talker) {
        return false;
    }

    float dx = talker->GetPosition().x - listener->GetPosition().x;
    float dy = talker->GetPosition().y - listener->GetPosition().y;
    float distanceSq = dx*dx + dy*dy;
    float maxDistanceSq = maxDistance * maxDistance;

    return distanceSq <= maxDistanceSq;
}

void SimpleVoiceChat::setUserVolume(uint32_t userID, float volume) {
    m_userVolumes[userID] = std::clamp(volume, 0.0f, 1.0f);
}

void SimpleVoiceChat::setUserMuted(uint32_t userID, bool muted) {
    m_userMuted[userID] = muted;
}

bool SimpleVoiceChat::isUserSpeaking(uint32_t userID) const {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    auto it = m_lastHeardTime.find(userID);
    if (it != m_lastHeardTime.end()) {
        return (now - it->second) < SPEAK_TIMEOUT_MS;
    }
    return false;
}

// We need to modify the network manager to check hearing range before processing voice packets
// This will be done in the network_manager.cpp file