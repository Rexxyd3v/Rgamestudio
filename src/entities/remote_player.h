#ifndef REMOTE_PLAYER_H
#define REMOTE_PLAYER_H

#include "character.h"
#include <cstdint>
#include <string>

// A RemotePlayer is a character whose position/state comes from the network.
// It does NOT read keyboard input. The server sends us its position and we just draw it.
class RemotePlayer : public Character {
public:
    RemotePlayer(Vector2 startPos, const std::string& assetPath, int weaponSkin = 0);
    ~RemotePlayer() override;

    // Called every frame with the latest data received from network
    void ApplyNetworkUpdate(Vector2 pos, int state, int weaponIndex, int faceDir, float hp, float jumpH, float jumpV);

    // Public wrapper around the protected Character::SetWeaponSkin so external
    // code (e.g. GameplayScreen) can update a remote player's weapon skin.
    void SetRemoteWeaponSkin(int weaponSkin) { SetWeaponSkin(weaponSkin); }

    // We implement Update but it only updates animation, not input
    void Update(float deltaTime) override;

    uint32_t peerID; // Which network peer this player belongs to
    std::string username;
    int kills;
    int deaths;
    Vector2 lastAimDir; // Last known aiming direction (normalized)
};

#endif // REMOTE_PLAYER_H
