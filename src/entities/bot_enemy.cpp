#include "bot_enemy.h"
#include <cmath>
#include <cstdlib>


const float BOT_SPEED = 100.0f;
const float BOT_SHOOT_COOLDOWN = 1.5f;
const float BOT_AGGRO_RANGE = 800.0f;
const float BOT_MOVE_RANGE = 500.0f; 
BotEnemy::BotEnemy(Vector2 startPosition, const std::string& assetPath)
    : Character(startPosition, assetPath, 0.08f), stateTimer(0.0f), isMoving(false), deathTimer(0.0f) {
    this->speed = BOT_SPEED;
    this->shootCooldown = BOT_SHOOT_COOLDOWN;
    this->health = 100.0f;
    this->currentWeaponIndex = GetRandomValue(0, 2); // Random weapon for variety
    this->targetPosition = startPosition;
}

BotEnemy::~BotEnemy() {
}

// Predefined spawn points covering the 3900x2040 world map
static const Vector2 SPAWN_POINTS[] = {
    { 300.0f, 300.0f },
    { 1800.0f, 300.0f },
    { 3500.0f, 300.0f },
    { 300.0f, 1000.0f },
    { 1800.0f, 1000.0f },
    { 3500.0f, 1000.0f },
    { 300.0f, 1700.0f },
    { 1800.0f, 1700.0f },
    { 3500.0f, 1700.0f }
};
static const int NUM_SPAWN_POINTS = sizeof(SPAWN_POINTS) / sizeof(SPAWN_POINTS[0]);

void BotEnemy::Update(float deltaTime) {
    // Standard updates like physics, animations, and projectiles
    if (IsDead()) {
        SetState(CharState::DEATH);
        deathTimer += deltaTime;
    } else {
        deathTimer = 0.0f;
    }
    
    UpdateSkills(deltaTime);
    UpdatePhysics(deltaTime);
    
    // Update cooldowns
    if (currentShootCooldown > 0.0f) {
        currentShootCooldown -= deltaTime;
    }
    
    // Update active projectiles
    for (auto& p : projectiles) {
        p->Update(deltaTime);
    }
    
    // Update current animation
    if (animations.count(currentState)) {
        animations[currentState]->Update(deltaTime);
    }
}

void BotEnemy::UpdateAI(float deltaTime, Vector2 playerPosition) {
    if (IsDead()) return;

    // Distance to target
    float dx = playerPosition.x - position.x;
    float dy = playerPosition.y - position.y;
    float distance = sqrtf(dx*dx + dy*dy);

    // Aim at target
    SetAimTarget(playerPosition);

    // Face the target
    faceDirection = (playerPosition.x > position.x) ? 1 : -1;

    if (isMonster) {
        // --- MONSTER MELEE AI: always chase aggressively, no shooting ---
        // Monsters are not allowed to use shield or dash — those skills are reserved for players and ranged bots.
        float monsterSpeed = speed * 1.8f; // faster than bots

        if (distance > 45.0f) {
            velocity.x = (dx / distance) * monsterSpeed;
            velocity.y = (dy / distance) * monsterSpeed;
            SetState(CharState::WALK);
        } else {
            velocity.x = 0;
            velocity.y = 0;
            SetState(CharState::IDLE);
        }
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        // Melee damage is handled in main_level.cpp (CheckCollisions / contact damage)
    } else {
        // --- BOT RANGED AI ---
        stateTimer -= deltaTime;
        if (stateTimer <= 0.0f) {
            stateTimer = (float)GetRandomValue(10, 25) / 10.0f;

            if (distance > BOT_MOVE_RANGE) {
                isMoving = true;
                float randomOffsetX = (float)GetRandomValue(-200, 200);
                float randomOffsetY = (float)GetRandomValue(-200, 200);
                targetPosition = {playerPosition.x + randomOffsetX, playerPosition.y + randomOffsetY};
            } else {
                isMoving = false;
            }
        }

        if (isMoving) {
            float tDx = targetPosition.x - position.x;
            float tDy = targetPosition.y - position.y;
            float tDist = sqrtf(tDx*tDx + tDy*tDy);

            if (tDist > 10.0f) {
                velocity.x = (tDx / tDist) * speed;
                velocity.y = (tDy / tDist) * speed;
                SetState(CharState::WALK);
            } else {
                isMoving = false;
                velocity.x = 0;
                velocity.y = 0;
                SetState(CharState::IDLE);
            }
        } else {
            velocity.x = 0;
            velocity.y = 0;
            SetState(CharState::IDLE);
        }

        // Random jump while moving
        bool isOnGround = (jumpHeight <= floorHeight + 1.0f && jumpVelocity <= 0.0f);
        bool startedJump = false;
        if (isMoving && isOnGround && GetRandomValue(0, 200) == 0) {
            jumpVelocity = 420.0f;
            startedJump = true;
        }

        // Random dash towards target
        if (isMoving && distance > 300.0f && GetRandomValue(1, 120) == 1) {
            ActivateDash({dx, dy});
        }
        // Shield if low health
        if (health < 40.0f && GetRandomValue(1, 100) == 1) {
            ActivateShield();
        }

        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;

        // Shoot if in aggro range
        if (distance < BOT_AGGRO_RANGE && currentShootCooldown <= 0.0f) {
            Shoot(playerPosition);
            currentShootCooldown = BOT_SHOOT_COOLDOWN + ((float)GetRandomValue(0, 10) / 20.0f);
        }

        // ---- JUMP / FALL state picking (override walk/idle) ----
        if (!isOnGround && jumpVelocity > 0.0f) {
            if (startedJump || currentState != CharState::JUMP_START) {
                SetState(CharState::JUMP_START);
            }
        } else if (!isOnGround && jumpVelocity <= 0.0f) {
            SetState(CharState::FALL);
        } else if (isOnGround && (currentState == CharState::JUMP_START || currentState == CharState::FALL)) {
            SetState(CharState::JUMP_END);
        }
    }
}

bool BotEnemy::ShouldRespawn() const {
    return IsDead() && deathTimer >= 3.0f;
}

void BotEnemy::ResetDeathTimer() {
    deathTimer = 0.0f;
    isMoving = false;
}
