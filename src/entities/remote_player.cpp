#include "remote_player.h"

static const float REMOTE_CHAR_SCALE = 0.08f;

static const std::string CHAR_PATH = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";

RemotePlayer::RemotePlayer(Vector2 startPos, const std::string& assetPath)
    : Character(startPos, assetPath, 0.08f), peerID(0), username("Player"), kills(0), deaths(0) {
    // We could tint remote players differently if we wanted
}

RemotePlayer::~RemotePlayer() {
}

void RemotePlayer::ApplyNetworkUpdate(Vector2 pos, int state, int weaponIndex, int faceDir, float hp, float jumpH, float jumpV) {
    // TEST: Direct assignment to position
    position.x = pos.x;
    position.y = pos.y;

    // Set animation state directly
    CharState newState = (CharState)state;
    if (newState != currentState) {
        SetState(newState);
    }

    // Set weapon
    if (weaponIndex >= 0 && weaponIndex < (int)weaponTextures.size()) {
        currentWeaponIndex = weaponIndex;
    }

    faceDirection = faceDir;
    health = hp;
    jumpHeight = jumpH;
    jumpVelocity = jumpV;
    // Point gun in the correct facing direction so it draws correctly
    aimTarget = { position.x + faceDir * 200.0f, position.y };
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
}
