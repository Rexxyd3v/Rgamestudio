#include "player.h"
#include <math.h>

static const float CHAR_SCALE = 0.15f;
static const std::string DEFAULT_CHAR_PATH =
    "assets/Free 2D Animated Vector Game Character Sprites/"
    "Free 2D Animated Vector Game Character Sprites/"
    "Full body animated characters/Char 1/with hands/";

Player::Player(Vector2 startPosition, int playerIndex, const std::string& skinPath, int weaponSkin)
    : Character(startPosition,
                skinPath.empty() ? DEFAULT_CHAR_PATH : skinPath,
                CHAR_SCALE,
                weaponSkin),
      playerIndex(playerIndex) {
}

Player::~Player() {}

void Player::Update(float deltaTime) {
    if (IsDead()) {
        SetState(CharState::DEATH);
        UpdatePhysics(deltaTime);
        animations[currentState]->Update(deltaTime);
        return;
    }

    UpdateSkills(deltaTime);

    if (currentShootCooldown > 0) currentShootCooldown -= deltaTime;

    // ---- JUMP ----
    bool isOnGround = (jumpHeight <= floorHeight + 1.0f && jumpVelocity <= 0.0f);
    bool startedJumpThisFrame = false;
    if (IsKeyPressed(KEY_SPACE) && isOnGround) {
        jumpVelocity = 420.0f;
        startedJumpThisFrame = true;
    }
    UpdatePhysics(deltaTime);

    // ---- MOVEMENT ----
    velocity  = { 0.0f, 0.0f };
    bool moving = false;

    if (playerIndex == 0) {
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    { velocity.y = -speed; moving = true; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))   { velocity.y =  speed; moving = true; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))   { velocity.x = -speed; moving = true; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))  { velocity.x =  speed; moving = true; }
    } else {
        if (IsKeyDown(KEY_I))  { velocity.y = -speed; moving = true; }
        if (IsKeyDown(KEY_K))  { velocity.y =  speed; moving = true; }
        if (IsKeyDown(KEY_J))  { velocity.x = -speed; moving = true; }
        if (IsKeyDown(KEY_L))  { velocity.x =  speed; moving = true; }
    }

    if (velocity.x != 0.0f && velocity.y != 0.0f) {
        velocity.x *= 0.7071f;
        velocity.y *= 0.7071f;
    }

    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
        ActivateDash(velocity);
    }
    if (IsKeyPressed(KEY_E)) {
        ActivateShield();
    }

    if (dashTimer <= 0.0f) {
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
    }

    // ---- STATE PICKING ----
    if (!isOnGround) {
        if (jumpVelocity > 0.0f) {
            if (startedJumpThisFrame || currentState != CharState::JUMP_START) {
                SetState(CharState::JUMP_START);
            }
        } else {
            if (currentState != CharState::FALL) {
                SetState(CharState::FALL);
            }
        }
    } else {
        bool jumpEndDone = (currentState == CharState::JUMP_END &&
                            animations.count(CharState::JUMP_END) &&
                            animations[CharState::JUMP_END]->IsFinished());

        if (currentState == CharState::JUMP_START || currentState == CharState::FALL) {
            SetState(CharState::JUMP_END);
        } else if (currentState == CharState::JUMP_END && !jumpEndDone) {
            // Still playing JUMP_END
        } else {
            if (moving) SetState(CharState::WALK);
            else        SetState(CharState::IDLE);
        }
    }

    // ---- AIM ----
    if (aimTarget.x < position.x) faceDirection = -1;
    else                          faceDirection =  1;

    // ---- SHOOT & WEAPONS ----
    if (playerIndex == 0) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyDown(KEY_Z)) Shoot(aimTarget);
        if (IsKeyPressed(KEY_R)) TriggerReload();
        if (IsKeyPressed(KEY_ONE)) SwitchWeapon(0);
        if (IsKeyPressed(KEY_TWO)) SwitchWeapon(1);
        if (IsKeyPressed(KEY_THREE)) SwitchWeapon(2);
    } else {
        if (IsKeyDown(KEY_M) || IsKeyDown(KEY_KP_0)) Shoot(aimTarget);
        if (IsKeyPressed(KEY_R)) TriggerReload();
        if (IsKeyPressed(KEY_SEVEN)) SwitchWeapon(0);
        if (IsKeyPressed(KEY_EIGHT)) SwitchWeapon(1);
        if (IsKeyPressed(KEY_NINE)) SwitchWeapon(2);
    }

    animations[currentState]->Update(deltaTime);

    for (auto it = projectiles.begin(); it != projectiles.end();) {
        (*it)->Update(deltaTime);
        if (!(*it)->IsActive()) it = projectiles.erase(it);
        else                    ++it;
    }
}
