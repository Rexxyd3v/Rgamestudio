#include "character.h"
#include "../utils/texture_manager.h"
#include <math.h>
#include <iostream>

Character::Character(Vector2 startPosition, const std::string& assetPath, float scale)
    : position(startPosition), faceDirection(1), currentState(CharState::IDLE),
      scale(scale), health(100.0f), speed(150.0f), shootCooldown(0.3f), currentShootCooldown(0.0f),
      jumpHeight(0.0f), jumpVelocity(0.0f), baseHeight(0.0f), floorHeight(0.0f), weaponRotation(0.0f), justShot(false) {

    aimTarget = { startPosition.x + 100.0f, startPosition.y };
    velocity  = { 0.0f, 0.0f };
    LoadAnimations(assetPath);
    
    currentWeaponIndex = 0;
    std::string weaponPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Weapons/";
    weaponTextures.push_back(TextureManager::GetTexture(weaponPath + "weaponR1.png"));
    weaponTextures.push_back(TextureManager::GetTexture(weaponPath + "weaponR2.png"));
    weaponTextures.push_back(TextureManager::GetTexture(weaponPath + "weaponR3.png"));
}

Character::~Character() {
    for (auto& pair : animations) delete pair.second;
}

void Character::LoadAnimations(const std::string& baseDir) {
    animations[CharState::IDLE]  = new Animation(baseDir + "idle_",  6,  0.10f, true);
    animations[CharState::WALK]  = new Animation(baseDir + "walk_",  8,  0.08f, true);
    animations[CharState::DEATH] = new Animation(baseDir + "death_", 10, 0.10f, false);
}

void Character::SetState(CharState newState) {
    if (currentState != newState) {
        currentState = newState;
        animations[currentState]->Reset();
    }
}

void Character::TakeDamage(float amount) {
    health -= amount;
    if (health <= 0) {
        health = 0;
        SetState(CharState::DEATH);
    }
}

float Character::GetHealth() const { return health; }

void Character::SwitchWeapon(int index) {
    if (index >= 0 && index < weaponTextures.size()) {
        currentWeaponIndex = index;
    }
}

// World-space position of the character's weapon hand
Vector2 Character::GetHandPosition() const {
    float draw_y = position.y - jumpHeight;
    // Sprite is 2048x2048. Hand anchor at approx (1170, 1580) in the right-facing sprite.
    float hand_x = position.x + (1170.0f - 1024.0f) * scale * faceDirection;
    float hand_y = draw_y      + (1580.0f - 1024.0f) * scale;
    return { hand_x, hand_y };
}

bool Character::Shoot(Vector2 targetPos) {
    if (IsDead()) return false;
    if (currentShootCooldown <= 0.0f) {
        currentShootCooldown = shootCooldown;

        Vector2 hand = GetHandPosition();
        float dx = targetPos.x - hand.x;
        float dy = targetPos.y - hand.y;
        float len = sqrtf(dx*dx + dy*dy);

        if (len > 1.0f) {
            Vector2 dir = { dx / len, dy / len };
            // Muzzle: offset a bit along the aim direction from the hand
            float muzzleOff = 20.0f;
            Vector2 projPos = { hand.x + dir.x * muzzleOff,
                                hand.y + dir.y * muzzleOff };
            projectiles.push_back(std::make_shared<Projectile>(projPos, dir, 700.0f));
            justShot = true;
            return true;
        }
    }
    return false;
}

// Used by RemotePlayer: spawn a projectile using a pre-computed direction (no cooldown check)
void Character::ShootInDirection(Vector2 dir) {
    if (IsDead()) return;
    Vector2 hand = GetHandPosition();
    float muzzleOff = 20.0f;
    Vector2 projPos = { hand.x + dir.x * muzzleOff, hand.y + dir.y * muzzleOff };
    projectiles.push_back(std::make_shared<Projectile>(projPos, dir, 700.0f));
}

void Character::Draw() {
    float draw_y = position.y - jumpHeight;
    animations[currentState]->Draw({ position.x, draw_y }, faceDirection, scale);

    if (!IsDead() && weaponTextures.size() > 0 && weaponTextures[currentWeaponIndex].id != 0) {
        Texture2D currentTex = weaponTextures[currentWeaponIndex];
        float weaponScale = scale * 0.45f;
        Vector2 hand = GetHandPosition();

        float dx = aimTarget.x - hand.x;
        float dy = aimTarget.y - hand.y;
        float rotation = 0.0f;
        if (fabsf(dx) > 0.01f || fabsf(dy) > 0.01f) {
            float angleDeg = atan2f(dy, dx) * RAD2DEG;
            rotation = (faceDirection == 1) ? angleDeg : angleDeg - 180.0f;
        }

        float pivotX = (faceDirection == 1)
                       ? 960.0f * weaponScale
                       : (2048.0f - 960.0f) * weaponScale;
        float pivotY = 1580.0f * weaponScale;

        Rectangle srcRec = { 0.0f, 0.0f,
                              (float)currentTex.width  * faceDirection,
                              (float)currentTex.height };
        Rectangle dstRec = { hand.x, hand.y,
                              (float)currentTex.width  * weaponScale,
                              (float)currentTex.height * weaponScale };
        Vector2 origin = { pivotX, pivotY };
        DrawTexturePro(currentTex, srcRec, dstRec, origin, rotation, WHITE);
    }

    for (auto& proj : projectiles) proj->Draw();
}

bool Character::IsAnimationFinished() const {
    auto it = animations.find(currentState);
    return (it != animations.end()) ? it->second->IsFinished() : true;
}

void Character::UpdatePhysics(float deltaTime) {
    // Apply vertical physics using floorHeight as the landing level
    if (jumpHeight > floorHeight || jumpVelocity > 0.0f) {
        jumpVelocity -= 950.0f * deltaTime; // gravity
        jumpHeight   += jumpVelocity * deltaTime;
        if (jumpHeight <= floorHeight) {
            jumpHeight   = floorHeight;
            jumpVelocity = 0.0f;
        }
    } else if (jumpHeight < floorHeight) {
        // Floor was raised (landed on platform) — snap up
        jumpHeight   = floorHeight;
        jumpVelocity = 0.0f;
    }
}
