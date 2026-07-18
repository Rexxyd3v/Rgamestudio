#include "player.h"
#include <math.h>

static const float CHAR_SCALE = 0.08f;
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
        animations[currentState]->Update(deltaTime);
        return;
    }

    UpdateSkills(deltaTime);

    if (currentShootCooldown > 0) currentShootCooldown -= deltaTime;

    // ---- JUMP ----
    // Allow jumping from both flat ground (floorHeight==0) and on a platform
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
        // Player 1: WASD
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    { velocity.y = -speed; moving = true; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))   { velocity.y =  speed; moving = true; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))   { velocity.x = -speed; moving = true; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))  { velocity.x =  speed; moving = true; }
    } else {
        // Player 2: IJKL
        if (IsKeyDown(KEY_I))  { velocity.y = -speed; moving = true; }
        if (IsKeyDown(KEY_K))  { velocity.y =  speed; moving = true; }
        if (IsKeyDown(KEY_J))  { velocity.x = -speed; moving = true; }
        if (IsKeyDown(KEY_L))  { velocity.x =  speed; moving = true; }
    }

    // Normalize diagonal movement
    if (velocity.x != 0.0f && velocity.y != 0.0f) {
        velocity.x *= 0.7071f;
        velocity.y *= 0.7071f;
    }

    // ---- SKILLS ----
    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
        ActivateDash(velocity);
    }
    if (IsKeyPressed(KEY_E)) {
        ActivateShield();
    }

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // World boundary (matches WORLD_WIDTH/HEIGHT in gameplay_screen.h)
    if (position.x < 30.0f)    position.x = 30.0f;
    if (position.x > 3870.0f)  position.x = 3870.0f;
    if (position.y < 30.0f)    position.y = 30.0f;
    if (position.y > 2010.0f)  position.y = 2010.0f;

    // ---- STATE PICKING (jump takes priority over walk/idle) ----
    if (!isOnGround) {
        if (jumpVelocity > 0.0f) {
            // Going up
            if (startedJumpThisFrame || currentState != CharState::JUMP_START) {
                SetState(CharState::JUMP_START);
            }
        } else {
            // Falling
            if (currentState != CharState::FALL) {
                SetState(CharState::FALL);
            }
        }
    } else {
        // On the ground: leave JUMP_END once it finishes, then pick walk or idle
        bool jumpEndDone = (currentState == CharState::JUMP_END &&
                            animations.count(CharState::JUMP_END) &&
                            animations[CharState::JUMP_END]->IsFinished());

        if (currentState == CharState::JUMP_START || currentState == CharState::FALL) {
            // Just landed — play the JUMP_END animation
            SetState(CharState::JUMP_END);
        } else if (currentState == CharState::JUMP_END && !jumpEndDone) {
            // Still playing JUMP_END; let it finish before transitioning
        } else {
            // JUMP_END finished, or we were idle/walk — pick walk or idle
            if (moving) SetState(CharState::WALK);
            else        SetState(CharState::IDLE);
        }
    }

    // ---- AIM ----
    // aimTarget is set externally (from gameplay_screen) for player 0 using mouse world coords.
    // For player 1 it's also set externally (auto-aimed at nearest enemy).
    if (aimTarget.x < position.x) faceDirection = -1;
    else                           faceDirection =  1;

    // ---- SHOOT, RELOAD & SWITCH WEAPONS ----
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
