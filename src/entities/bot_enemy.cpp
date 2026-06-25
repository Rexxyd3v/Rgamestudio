#include "bot_enemy.h"
#include <cmath>
#include <cstdlib>

// Bot constants
const float BOT_SPEED = 100.0f;
const float BOT_SHOOT_COOLDOWN = 1.5f;
const float BOT_AGGRO_RANGE = 800.0f; // Range at which bot shoots
const float BOT_MOVE_RANGE = 500.0f;  // Range at which bot stops moving towards player

BotEnemy::BotEnemy(Vector2 startPosition, const std::string& assetPath)
    : Character(startPosition, assetPath, 0.08f), stateTimer(0.0f), isMoving(false) {
    this->speed = BOT_SPEED;
    this->shootCooldown = BOT_SHOOT_COOLDOWN;
    this->health = 100.0f;
    this->currentWeaponIndex = GetRandomValue(0, 2); // Random weapon for variety
    this->targetPosition = startPosition;
}

BotEnemy::~BotEnemy() {
}

void BotEnemy::Update(float deltaTime) {
    // Standard updates like physics, animations, and projectiles
    if (IsDead()) {
        SetState(CharState::DEATH);
    }
    
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

    // Distance to player
    float dx = playerPosition.x - position.x;
    float dy = playerPosition.y - position.y;
    float distance = sqrtf(dx*dx + dy*dy);

    // Aim at player
    SetAimTarget(playerPosition);

    // AI Logic
    stateTimer -= deltaTime;
    if (stateTimer <= 0.0f) {
        // Decide what to do every ~1-2 seconds
        stateTimer = (float)GetRandomValue(10, 25) / 10.0f;
        
        if (distance > BOT_MOVE_RANGE) {
            // Player is far, move towards them
            isMoving = true;
            
            // Give some randomness to the movement so they don't walk in a perfect straight line
            float randomOffsetX = (float)GetRandomValue(-200, 200);
            float randomOffsetY = (float)GetRandomValue(-200, 200);
            targetPosition = {playerPosition.x + randomOffsetX, playerPosition.y + randomOffsetY};
        } else {
            // Player is close enough, stop moving and just shoot
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
            
            // Set facing direction based on movement if not aggressively aiming, 
            // but for a shooter it's better to face the aim target
            if (playerPosition.x > position.x) {
                faceDirection = 1;
            } else {
                faceDirection = -1;
            }
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
        
        // Face player
        if (playerPosition.x > position.x) {
            faceDirection = 1;
        } else {
            faceDirection = -1;
        }
    }

    // Apply movement
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Shoot if in range and cooldown is ready
    if (distance < BOT_AGGRO_RANGE && currentShootCooldown <= 0.0f) {
        Shoot(playerPosition);
        // Slightly random cooldown to avoid all bots shooting at the exact same frame
        currentShootCooldown = BOT_SHOOT_COOLDOWN + ((float)GetRandomValue(0, 10) / 20.0f); 
    }
}
