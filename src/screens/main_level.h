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
    Texture2D bgDetail; // pebble detail layer
    Texture2D rockTex;  // rock3
    Texture2D rockTex1; // rock1
    Texture2D rockTex2; // rock2
    Texture2D healthBarFrames[9];  // Animated health bar: index 0=frame1 (lowest), index 8=frame9 (full)
    Texture2D dashBarFrames[9];    // Animated dash bar:   index 0=dash1.png (full), index 8=dash9.png (empty)
    Texture2D headPortrait;        // Character head portrait shown in HUD circle
    float worldTime;    // accumulates delta for animations
    float lastDashCooldown = 0.0f; // tracks transition 0 -> 2.0 to detect a fresh dash

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
    Vector2 GetFarSpawnPoint();
    Character* GetNearestTargetForBot(BotEnemy* b);
};

#endif
