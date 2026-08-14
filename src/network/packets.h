#ifndef PACKETS_H
#define PACKETS_H

// Must define these before any Windows headers to avoid conflicts with Raylib
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <raylib.h>
#include <stdint.h>

enum class PacketType : uint8_t {
    ID_ASSIGNMENT = 0,  // Host -> Client: grants the authoritative playerID
    PLAYER_CONNECT,     // Player info announcement (used for both new and existing)
    PLAYER_DISCONNECT,
    PLAYER_READY,       // Player ready state changed
    PLAYER_UPDATE,      // Sent constantly: position, state, weapon, facing
    PLAYER_SHOOT,       // Event: player fired a shot
    PLAYER_DAMAGE,      // Event: player took damage
    PLAYER_KILLED,      // Event: player was killed by another (updates score)
    PLAYER_RESPAWN,     // Event: player respawned
    GAME_START,         // Host signals game should start
    MAP_CHANGED,        // Host signals selected map changed
    VOICE_DATA          // Voice audio data (Opus encoded)
};

// Base header for all packets
#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    uint32_t playerID;  // Authoritative player ID assigned by the host. 0 = host, >0 = clients.
};

// Host -> single client: "your authoritative playerID is X"
struct PacketIDAssignment {
    PacketHeader header;
    uint32_t assignedID;   // The host-assigned playerID
};

// Host -> all clients: announcement that a player exists.
// This packet is ONLY emitted by the host. The header.playerID is authoritative.
// Clients NEVER generate or modify playerIDs.
struct PacketPlayerConnect {
    PacketHeader header;
    char username[20];
    int charSkin;     // 1, 2, 3, or 4
    int weaponSkin;   // WeaponSkinId enum value
};

struct PacketPlayerUpdate {
    PacketHeader header;
    Vector2 position;
    int state;             // CharState (Idle, Walk, Death, etc)
    int currentWeaponIndex;
    int faceDirection;     // 1 or -1
    float health;
    float jumpHeight;      // Vertical jump sync
    float jumpVelocity;
    char username[16];
    int charSkin;
    int weaponSkin;        // WeaponSkinId enum value
};

struct PacketPlayerShoot {
    PacketHeader header;
    Vector2 aimDir;   // Normalized direction vector (not world position)
};

struct PacketPlayerDamage {
    PacketHeader header;
    uint32_t targetPlayerID; // Authoritative ID of damaged player (assigned by host)
    float damageAmount;
};

struct PacketPlayerKilled {
    PacketHeader header;
    uint32_t killerPlayerID; // Authoritative ID of killer
    uint32_t victimPlayerID; // Authoritative ID of victim
};

struct PacketPlayerRespawn {
    PacketHeader header;
    Vector2 spawnPosition;
    int charSkin;  // Skin ID (1-4)
    int weaponSkin; // WeaponSkinId enum value
};

struct PacketPlayerReady {
    PacketHeader header;
    bool isReady;  // True if player is ready, false otherwise
};

struct PacketPlayerDisconnectHeader {
    PacketHeader header;
    // No additional data needed for disconnect notification
};

struct PacketGameStart {
    PacketHeader header;
    char mapName[64]; // Map folder name from MapRegistry
};

struct PacketMapChanged {
    PacketHeader header;
    char mapName[64]; // Map folder name from MapRegistry
};

// Voice data packet (Opus encoded audio)
#define MAX_OPUS_FRAME_SIZE 1200 // Maximum size of an Opus frame in bytes
struct PacketVoiceData {
    PacketHeader header;
    uint16_t sequenceNumber; // For detecting packet loss/reordering
    uint16_t frameSize;      // Size of the Opus frame in bytes
    uint8_t  opusData[MAX_OPUS_FRAME_SIZE]; // The encoded audio data
};
#pragma pack(pop)

#endif // PACKETS_H
