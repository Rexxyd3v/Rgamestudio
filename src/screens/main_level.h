#ifndef GAMEPLAY_SCREEN_H
#define GAMEPLAY_SCREEN_H

#include <raylib.h>
#include "iscreen.h"
#include "../entities/player.h"
#include "../entities/remote_player.h"
#include "../entities/bot_enemy.h"
#include <vector>
#include <memory>
#include "../constants.h"

// World is 3x the virtual screen
#define WORLD_WIDTH  3900
#define WORLD_HEIGHT 2040

class GameplayScreen : public IScreen {
public:
    explicit GameplayScreen(GameMode mode);
    ~GameplayScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;

private:
    struct Rock {
        Vector2 position;   // center in world coords
        float   scale;      // draw scale of the rock sprite
        float   radius;     // side-collision radius
        float   visualHeight;
        float   platformTop;  // cached platform top Y
    };

    // Entities
    GameMode currentMode;
    Player* player;
    Player* player2;
    std::vector<BotEnemy*> offlineBots;       // Offline AI opponents
    std::vector<RemotePlayer*> remotePlayers; // Online opponents
    std::vector<Rock> rocks;

    Camera2D camera;

    Texture2D bgTex1;
    Texture2D bgTex2;
    Texture2D rockTex;

    float spawnTimer;
    float netSendTimer;

    // Networking helpers
    void PollNetworkEvents(float deltaTime);
    RemotePlayer* FindOrCreateRemotePlayer(uint32_t playerID, int charSkin);
    void RemoveRemotePlayer(uint32_t playerID);

    // Physics & collision helpers
    void  ResolveRockCollisions(Character* c);
    void  CheckCollisions();
    Character* GetNearestEnemy(Vector2 pos);
    Character* GetNearestPlayerOrCompanion(Vector2 pos);
};

#endif
