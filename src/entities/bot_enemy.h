#ifndef BOT_ENEMY_H
#define BOT_ENEMY_H

#include "character.h"

class BotEnemy : public Character {
public:
    BotEnemy(Vector2 startPosition, const std::string& assetPath);
    ~BotEnemy() override;

    // AI logic injected via Update
    void Update(float deltaTime) override;
    
    // Provide a reference to the player to chase/shoot at
    void UpdateAI(float deltaTime, Vector2 playerPosition);

private:
    float stateTimer;
    Vector2 targetPosition; // Where to move
    bool isMoving;
};

#endif // BOT_ENEMY_H
