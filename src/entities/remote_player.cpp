#include "remote_player.h"

static const float CHAR_SCALE = 0.08f;


static const std::string CHAR_PATH = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";

RemotePlayer::RemotePlayer(Vector2 startPos, const std::string& assetPath, int weaponSkin)
    : Character(startPos, assetPath, CHAR_SCALE, weaponSkin), peerID(0), username("Player"), lastAimDir({0.0f, 0.0f}) {

    // Keep the inherited Character::name in sync so the head-label draw code
    // (which reads Character::name) works for online players too.
    Character::SetName(username);
}

RemotePlayer::~RemotePlayer() {
}

void RemotePlayer::ApplyNetworkUpdate(Vector2 pos, int state, int weaponIndex, int faceDir, float hp, float jumpH, float jumpV, int weaponSkin) {
    // TEST: Direct assignment to position
    position.x = pos.x;
    position.y = pos.y;

    // Set animation state directly
    CharState newState = (CharState)state;
    if (newState != currentState) {
        SetState(newState);
    }

    // Keep the remote player's weapon skin in sync with the networked selection.
    SetRemoteWeaponSkin(weaponSkin);

    // Set weapon
    if (weaponIndex >= 0 && weaponIndex < (int)weaponTextures.size()) {
        currentWeaponIndex = weaponIndex;
    }

    faceDirection = faceDir;
    health = hp;
    jumpHeight = jumpH;
    jumpVelocity = jumpV;
    // Aim direction is updated from shoot packets; aiming for rendering is handled in Update()
    // aimTarget = { position.x + faceDir * 200.0f, position.y };
}

void RemotePlayer::Update(float deltaTime) {
    UpdateSkills(deltaTime);

    // Only update the animation timer, no keyboard input
    if (animations.count(currentState)) {
        animations[currentState]->Update(deltaTime);
    }
    // Update projectiles
    for (int i = 0; i < (int)projectiles.size(); ) {
        projectiles[i]->Update(deltaTime);
        if (!projectiles[i]->IsActive()) {
            projectiles.erase(projectiles.begin() + i);
        } else {
            i++;
        }
    }
    // Aim direction for rendering: use last known aim direction from shoot packets
    // If no aim data yet, fallback to facing direction.
    Vector2 aimDir = lastAimDir;
    if (aimDir.x == 0.0f && aimDir.y == 0.0f) {
        aimDir = { (float)faceDirection, 0.0f };
    }
    // Aim target is a point in front of the character (used for weapon rotation)
    aimTarget = { position.x + aimDir.x * 200.0f, position.y + aimDir.y * 200.0f };
}
