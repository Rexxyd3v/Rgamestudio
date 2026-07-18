#include "voice_chat.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include "../network/network_manager.h"

// Opus error checking macro
#define OPUS_CHECK(call) \
    do { \
        int err = (call); \
        if (err < 0) { \
            std::cerr << "Opus error: " << opus_strerror(err) << " (" << #call << ")" << std::endl; \
        } \
    } while (0)

// Static sequence counter for voice packets
static uint16_t s_sequenceCounter = 0;

VoiceChat::VoiceChat()
    : m_encoder(nullptr)
    , m_transmitEnabled(false)
    , m_initialized(false)
    , m_baseTimestamp(0)
    , m_localSpeakUntilMs(0)
{
    // Initialize buffers
    m_captureBuffer.resize(FRAME_SIZE);
    m_mixBuffer.resize(FRAME_SIZE * 2); // Stereo mix buffer for processing
}

VoiceChat::~VoiceChat() {
    shutdown();
}

bool VoiceChat::initialize() {
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
    config.dataCallback       = captureCallback;
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
    config.dataCallback       = playbackCallback;
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
    std::cout << "Voice chat initialized successfully." << std::endl;
    return true;
}

bool VoiceChat::isLocalSpeaking() const {
    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return m_localSpeakUntilMs.load() > now;
}

bool VoiceChat::isUserSpeaking(uint32_t userID) const {
    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    auto it = m_lastHeardMs.find(userID);
    if (it == m_lastHeardMs.end()) return false;
    return it->second > now;
}

void VoiceChat::shutdown() {
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

    std::cout << "Voice chat shut down." << std::endl;
}

void VoiceChat::setTransmitEnabled(bool enabled) {
    m_transmitEnabled.store(enabled);

    // Local push-to-talk indicator
    if (enabled) {
        uint64_t now = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        // Show indicator for a short window (so it doesn't flicker on tiny pauses)
        m_localSpeakUntilMs.store(now + 250);
    }

    // Clear capture buffer when disabling transmission to avoid sending old audio
    if (!enabled) {
        std::fill(m_captureBuffer.begin(), m_captureBuffer.end(), 0);
    }
}

void VoiceChat::captureAudio() {
    // This function is called periodically from the main loop
    // The actual capture happens in the callback, but we can process the buffer here

    // Only process if transmission is enabled
    if (!m_transmitEnabled.load()) {
        return;
    }

    // In a more sophisticated implementation, we would:
    // 1. Check if there's new audio data available from the callback
    // 2. Apply voice activity detection
    // 3. Encode and send if voice is detected

    // For now, we'll rely on the callback to handle encoding and queuing for transmission
    // The callback will handle the actual encoding when new audio data arrives
}

void VoiceChat::processVoicePacket(const PacketVoiceData* packet, uint32_t senderID) {
    if (!m_initialized.load() || !packet) {
        return;
    }

    // Mark user as speaking for UI indicator
    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    m_lastHeardMs[senderID] = now;

    // Ensure decoder exists for this user
    auto decoderIt = m_decoders.find(senderID);
    if (decoderIt == m_decoders.end() || !decoderIt->second) {
        int opusError;
        OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusError);
        if (!decoder || opusError < 0) {
            std::cerr << "Failed to create Opus decoder for user " << senderID << ": "
                      << opus_strerror(opusError) << std::endl;
            return;
        }
        m_decoders[senderID] = decoder;
        m_lastSequenceNum[senderID] = 0;
        m_userVolumes[senderID] = 1.0f;
        m_userMuted[senderID] = false;
        decoderIt = m_decoders.find(senderID);
    }

    // Packet freshness / de-duplication
    const uint16_t currentSeq = packet->sequenceNumber;
    const uint16_t lastSeq = m_lastSequenceNum[senderID];

    bool packetIsNew = false;
    if (currentSeq >= lastSeq) {
        packetIsNew = (currentSeq - lastSeq < 32768);
    } else {
        packetIsNew = ((65535 - lastSeq) + currentSeq < 32768);
    }

    if (!packetIsNew) {
        return;
    }

    m_lastSequenceNum[senderID] = currentSeq;

    // Optional decode check (helps detect invalid packets). We don't mix PCM here;
    // we queue the Opus payload into the jitter buffer.
    {
        std::vector<int16_t> scratch(FRAME_SIZE);
        const int decodedFrames = opus_decode(
            decoderIt->second,
            packet->opusData,
            static_cast<opus_int32>(packet->frameSize),
            scratch.data(),
            FRAME_SIZE,
            0 // No PLC
        );

        if (decodedFrames <= 0) {
            // Invalid / undecodable frame
            return;
        }
    }

    // Push exactly one entry into the jitter buffer
    {
        std::lock_guard<std::mutex> lock(m_jitterMutex);
        VoicePacket vp;
        vp.senderID = senderID;
        vp.sequenceNumber = currentSeq;
        vp.opusData.assign(packet->opusData, packet->opusData + packet->frameSize);
        vp.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        m_jitterBuffer.push(std::move(vp));
    }

    m_jitterCondition.notify_one();
}


void VoiceChat::updatePlayback(float deltaTime) {
    if (!m_initialized.load()) {
        return;
    }

    // Process jitter buffer to play audio
    const uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::vector<VoicePacket> readyPackets;
    {
        std::unique_lock<std::mutex> lock(m_jitterMutex);

        // Wait for packets to arrive (non-blocking with timeout)
        if (m_jitterBuffer.empty()) {
            // Wait briefly for packets
            if (m_jitterCondition.wait_for(lock, std::chrono::milliseconds(1)) ==
                std::cv_status::timeout) {
                // Timeout - no new packets
            }
        }

        // Move all packets that are old enough to play
        while (!m_jitterBuffer.empty()) {
            VoicePacket& pkt = m_jitterBuffer.front();
            uint64_t age = now - pkt.timestamp;

            // Play packets that are at least 20ms old (to allow for jitter)
            if (age >= 20) {
                readyPackets.push_back(std::move(pkt));
                m_jitterBuffer.pop();
            } else {
                break; // Packet is too new, wait for it
            }
        }
    }

    // Decode and mix all ready packets
    if (!readyPackets.empty()) {
        // Clear mix buffer
        std::fill(m_mixBuffer.begin(), m_mixBuffer.end(), 0.0f);

        // Decode and mix each packet
        for (const VoicePacket& pkt : readyPackets) {
            // Skip if user is muted
            if (m_userMuted[pkt.senderID]) {
                continue;
            }

            // Decode Opus packet using the sender's specific decoder
            auto decoderIt = m_decoders.find(pkt.senderID);
            if (decoderIt == m_decoders.end() || !decoderIt->second) {
                // Create decoder for this user if it doesn't exist
                int opusError;
                OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusError);
                if (!decoder || opusError < 0) {
                    continue;
                }
                m_decoders[pkt.senderID] = decoder;
                m_lastSequenceNum[pkt.senderID] = 0;
                m_userVolumes[pkt.senderID] = 1.0f;
                m_userMuted[pkt.senderID] = false;
                decoderIt = m_decoders.find(pkt.senderID);
            }

            int decodedFrames = opus_decode(
                decoderIt->second,
                pkt.opusData.data(),
                static_cast<opus_int32>(pkt.opusData.size()),
                nullptr,
                0,
                0 // No PLC - first get the size
            );

            if (decodedFrames <= 0) {
                continue;
            }

            std::vector<int16_t> pcmData(decodedFrames);
            decodedFrames = opus_decode(
                decoderIt->second,
                pkt.opusData.data(),
                static_cast<opus_int32>(pkt.opusData.size()),
                pcmData.data(),
                decodedFrames,
                0 // No PLC
            );

            if (decodedFrames <= 0) {
                continue;
            }

            // Convert to float and apply volume
            float volume = m_userVolumes.count(pkt.senderID) ? m_userVolumes[pkt.senderID] : 1.0f;
            for (size_t i = 0; i < pcmData.size() && i < m_mixBuffer.size(); ++i) {
                // Convert from int16 to float (-1.0 to 1.0 range)
                float sample = static_cast<float>(pcmData[i]) / 32768.0f;
                m_mixBuffer[i] += sample * volume;
            }
        }

        // Clipping prevention
        for (float& sample : m_mixBuffer) {
            if (sample > 1.0f) sample = 1.0f;
            else if (sample < -1.0f) sample = -1.0f;
        }

        // The actual playback happens in the callback, which reads from m_mixBuffer
        // We just need to make sure the data is ready when the callback needs it
    }
}

// Static callback for audio capture
void VoiceChat::captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    VoiceChat* self = static_cast<VoiceChat*>(pDevice->pUserData);
    if (!self || !self->m_initialized.load()) {
        return;
    }

    // Only process if transmission is enabled
    if (!self->m_transmitEnabled.load()) {
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

    // Encode and queue for transmission
    if (self->m_encoder && self->m_captureBuffer.size() >= FRAME_SIZE) {
        std::vector<uint8_t> opusData = self->encodeOpus(self->m_captureBuffer.data(), FRAME_SIZE);

        if (!opusData.empty()) {
            // Create and send voice packet
            PacketVoiceData packet;
            packet.header.type = PacketType::VOICE_DATA;
            packet.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
            packet.sequenceNumber = ++s_sequenceCounter;
            packet.frameSize = opusData.size();
            std::memcpy(packet.opusData, opusData.data(), opusData.size());

            // Send via network manager (unreliable for voice)
            size_t packetSize = sizeof(PacketHeader) + sizeof(packet.sequenceNumber) + sizeof(packet.frameSize) + packet.frameSize;
            NetworkManager::GetInstance().SendPacket(&packet, packetSize, false); // Unreliable
        }
    }
}

// Static callback for audio playback
void VoiceChat::playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    VoiceChat* self = static_cast<VoiceChat*>(pDevice->pUserData);
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

std::vector<uint8_t> VoiceChat::encodeOpus(const int16_t* pcmData, size_t frameCount) {
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

void VoiceChat::mixAudio(float* output, const std::vector<float*>& inputs, size_t frameCount, size_t channelCount) {
    // Simple mixing implementation
    std::fill(output, output + frameCount * channelCount, 0.0f);

    for (const float* input : inputs) {
        for (size_t i = 0; i < frameCount * channelCount; ++i) {
            output[i] += input[i];
        }
    }

    // Prevent clipping
    for (size_t i = 0; i < frameCount * channelCount; ++i) {
        if (output[i] > 1.0f) output[i] = 1.0f;
        else if (output[i] < -1.0f) output[i] = -1.0f;
    }
}

void VoiceChat::setUserVolume(uint32_t userID, float volume) {
    m_userVolumes[userID] = std::clamp(volume, 0.0f, 1.0f);
}

void VoiceChat::setUserMuted(uint32_t userID, bool muted) {
    m_userMuted[userID] = muted;
}