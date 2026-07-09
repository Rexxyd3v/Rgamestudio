#include "main_level.h"
#include "../constants.h"
#include "../utils/texture_manager.h"
#include "../network/network_manager.h"
#include "../network/packets.h"
#include <iostream>
#include <math.h>
#include <algorithm>
#include <cstring>

// Simple clamp helper (raylib Clamp may not be available without raymath)
static inline float fclamp(float val, float lo, float hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}

static float netSendTimer = 0.0f;
static const float NET_SEND_RATE = 1.0f / 30.0f; // Send 30 updates per second

GameplayScreen::GameplayScreen(GameMode mode) : currentMode(mode), spawnTimer(0.0f), netSendTimer(0.0f), worldTime(0.0f) {
    std::string bgPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Environment/";
    bgTex1   = TextureManager::GetTexture(bgPath + "ground_white.png");
    bgTex2   = TextureManager::GetTexture(bgPath + "ground2_white.png");
    bgDetail = TextureManager::GetTexture(bgPath + "ground3_white.png");
    rockTex  = TextureManager::GetTexture(bgPath + "rock3.png");
    rockTex1 = TextureManager::GetTexture(bgPath + "rock1.png");
    rockTex2 = TextureManager::GetTexture(bgPath + "rock2.png");
    // Load all 9 health bar animation frames
    for (int i = 0; i < 9; i++) {
        healthBarFrames[i] = TextureManager::GetTexture(
            "assets/HealthBar/" + std::to_string(i + 1) + "_no_bg.png");
    }

    // Load all 9 dash bar animation frames
    for (int i = 0; i < 9; i++) {
        dashBarFrames[i] = TextureManager::GetTexture(
            "assets/DashBar/dash" + std::to_string(i + 1) + ".png");
    }

    // Load the character head portrait for the HUD circle (use the player's selected skin)
    {
        std::string charPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";
        int skinIdx = NetworkManager::GetInstance().localSkinIndex;
        if (skinIdx < 1 || skinIdx > 4) skinIdx = 1;
        std::string headPath = charPath + "Char " + std::to_string(skinIdx) + "/head.png";
        headPortrait = TextureManager::GetTexture(headPath);
        // Fallback to assets/Head_display/char1.png if skin-specific head not found
        if (headPortrait.id == 0) {
            headPortrait = TextureManager::GetTexture("assets/Head_display/char1.png");
        }
    }

    // Player starts near center of world, using selected skin
    {
        std::string charPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";
        int skinIdx = NetworkManager::GetInstance().localSkinIndex;
        if (skinIdx < 1 || skinIdx > 4) skinIdx = 1;
        std::string skinPath = charPath + "Char " + std::to_string(skinIdx) + "/with hands/";
        player = new Player({WORLD_WIDTH / 2.0f - 50.0f, WORLD_HEIGHT / 2.0f}, 0, skinPath);
    }
    // In online mode player2 is a remote player, so we don't spawn them locally
    // player2 is unused now - all opponents come via network as RemotePlayer
    player2 = nullptr;

    if (currentMode == GameMode::OFFLINE) {
        std::string charPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";
        
        Vector2 spawnPoints[] = {
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
        int numSpawns = sizeof(spawnPoints) / sizeof(spawnPoints[0]);

        for (int i = 0; i < numSpawns; i++) {
            int r = GetRandomValue(0, numSpawns - 1);
            Vector2 temp = spawnPoints[i];
            spawnPoints[i] = spawnPoints[r];
            spawnPoints[r] = temp;
        }

        // Spawn 4 Character Bots
        BotEnemy* b0 = new BotEnemy(spawnPoints[0], charPath + "Char 1/with hands/"); b0->SetName("Alpha Bot"); offlineBots.push_back(b0);
        BotEnemy* b1 = new BotEnemy(spawnPoints[1], charPath + "Char 2/with hands/"); b1->SetName("Bravo Bot"); offlineBots.push_back(b1);
        BotEnemy* b2 = new BotEnemy(spawnPoints[2], charPath + "Char 3/with hands/"); b2->SetName("Charlie Bot"); offlineBots.push_back(b2);
        BotEnemy* b3 = new BotEnemy(spawnPoints[3], charPath + "Char 4/with hands/"); b3->SetName("Delta Bot"); offlineBots.push_back(b3);

        // Spawn 4 Monster Enemies
        BotEnemy* m0 = new BotEnemy(spawnPoints[4], charPath + "Enemies/Enemy 1/"); m0->SetName("Ghoul"); offlineBots.push_back(m0);
        BotEnemy* m1 = new BotEnemy(spawnPoints[5], charPath + "Enemies/Enemy 2/"); m1->SetName("Zombie"); offlineBots.push_back(m1);
        BotEnemy* m2 = new BotEnemy(spawnPoints[6], charPath + "Enemies/Enemy 3/"); m2->SetName("Wraith"); offlineBots.push_back(m2);
        BotEnemy* m3 = new BotEnemy(spawnPoints[7], charPath + "Enemies/Enemy 4/"); m3->SetName("Demon"); offlineBots.push_back(m3);

        // Set player name and starting position to a unique spawnpoint
        player->SetName(NetworkManager::GetInstance().localUsername.empty() ? "You" : NetworkManager::GetInstance().localUsername);
        player->SetPosition(spawnPoints[8]);
    }

    // No AI companions in Online PvP mode
    // No AI companions needed - it's PvP!

    // Scatter rocks around the world with variety
    rocks = {
        // Rock type 3 (large angular - center areas)
        {{400.0f,   500.0f},  0.55f, 90.0f,  0.0f},
        {{900.0f,   900.0f},  0.75f, 120.0f, 0.0f},
        {{1600.0f,  400.0f},  0.45f, 70.0f,  0.0f},
        {{2200.0f,  1100.0f}, 0.65f, 105.0f, 0.0f},
        {{2800.0f,  700.0f},  0.50f, 80.0f,  0.0f},
        {{3400.0f,  1500.0f}, 0.70f, 112.0f, 0.0f},
        {{1100.0f,  1600.0f}, 0.45f, 72.0f,  0.0f},
        {{2600.0f,  350.0f},  0.55f, 88.0f,  0.0f},
        {{700.0f,   1400.0f}, 0.60f, 96.0f,  0.0f},
        {{3000.0f,  1700.0f}, 0.40f, 64.0f,  0.0f},
        // Extra coverage across large map
        {{500.0f,  1200.0f},  0.50f, 80.0f,  0.0f},
        {{1400.0f,  700.0f},  0.45f, 72.0f,  0.0f},
        {{3200.0f,  500.0f},  0.60f, 95.0f,  0.0f},
        {{1900.0f, 1800.0f},  0.55f, 88.0f,  0.0f},
        {{2400.0f, 1400.0f},  0.40f, 64.0f,  0.0f},
        {{150.0f,   750.0f},  0.50f, 80.0f,  0.0f},
        {{3700.0f, 1100.0f},  0.65f, 104.0f, 0.0f},
        {{1300.0f, 1200.0f},  0.45f, 72.0f,  0.0f},
        {{3100.0f, 1200.0f},  0.55f, 88.0f,  0.0f},
        {{2000.0f,  600.0f},  0.40f, 64.0f,  0.0f},
    };
    // Set platformTop: approximate top Y of each rock so characters can stand on it
    for (auto& r : rocks) {
        // rock texture center is at position; top surface is about radius above center
        r.platformTop = r.position.y - r.radius * 0.8f;
    }

    if (currentMode == GameMode::OFFLINE) HideCursor();

    // Camera2D
    camera.target   = player->GetPosition();
    camera.offset   = {VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;
}

GameplayScreen::~GameplayScreen() {
    delete player;
    if (player2) delete player2;
    for (auto b : offlineBots) delete b;
    for (auto r : remotePlayers) delete r;
    // healthBarFrames textures are managed by TextureManager
    if (currentMode == GameMode::OFFLINE) ShowCursor();
}

Character* GameplayScreen::GetNearestEnemy(Vector2 pos) {
    // In PvP mode, nearest "enemy" is actually a remote player
    Character* nearest = nullptr;
    float minDist = 999999.0f;
    for (auto r : remotePlayers) {
        if (r->IsDead()) continue;
        float dx = r->GetPosition().x - pos.x;
        float dy = r->GetPosition().y - pos.y;
        float dist = dx*dx + dy*dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = r;
        }
    }
    for (auto b : offlineBots) {
        if (b->IsDead()) continue;
        float dx = b->GetPosition().x - pos.x;
        float dy = b->GetPosition().y - pos.y;
        float dist = dx*dx + dy*dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = b;
        }
    }
    return nearest;
}

// Resolve a character being pushed away from rocks (circular collision)
void GameplayScreen::ResolveRockCollisions(Character* c) {
    // NOTE: legacy function body removed because its declaration helper was being removed.
    // If you still need rock collisions, restore this implementation.
    (void)c;
}


static const float HIT_HALF_W   = 22.0f;   // half-width of visible body in world units
static const float HIT_TOP_OFF  =  0.0f;   // body top relative to sprite center
static const float HIT_BOT_OFF  = 60.0f;   // body bottom relative to sprite center (feet below)

// Projectile radius (small circle around the bullet)
static const float PROJ_RADIUS  = 3.0f;

// Returns true if projectile at `pp` hits the AABB for a character whose
// visual center is at `cc` (already adjusted for jumpHeight).
static inline bool ProjectileHitsAABB(Vector2 pp, Vector2 cc) {
    float left   = cc.x - HIT_HALF_W;
    float right  = cc.x + HIT_HALF_W;
    float top    = cc.y + HIT_TOP_OFF;
    float bottom = cc.y + HIT_BOT_OFF;

    // Find closest point on the AABB to the projectile (sweep with small bullet radius)
    float cx = (pp.x < left) ? left : (pp.x > right ? right : pp.x);
    float cy = (pp.y < top)  ? top  : (pp.y > bottom ? bottom : pp.y);

    float dx = pp.x - cx;
    float dy = pp.y - cy;
    return (dx*dx + dy*dy) <= PROJ_RADIUS * PROJ_RADIUS;
}

void GameplayScreen::CheckCollisions() {
    // Player 1 projectiles hitting remote players (online PvP)
    std::vector<std::shared_ptr<Projectile>>& playerProjs = player->GetProjectiles();
    for (int i = 0; i < (int)playerProjs.size(); i++) {
        if (!playerProjs[i]->IsActive()) continue;
        Vector2 projPos = playerProjs[i]->GetPosition();
        for (int j = 0; j < (int)remotePlayers.size(); j++) {
            RemotePlayer* t = remotePlayers[j];
            if (t->IsDead()) continue;
            Vector2 cc = { t->GetPosition().x, t->GetPosition().y - t->GetJumpHeight() };
            if (ProjectileHitsAABB(projPos, cc)) {
                PacketPlayerDamage dmgPkt;
                dmgPkt.header.type    = PacketType::PLAYER_DAMAGE;
                dmgPkt.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
                dmgPkt.targetPlayerID = t->peerID;
                dmgPkt.damageAmount   = playerProjs[i]->GetDamage();
                NetworkManager::GetInstance().SendPacket(&dmgPkt, sizeof(dmgPkt), true);
                t->TakeDamage(playerProjs[i]->GetDamage());
                playerProjs[i]->Deactivate();
                break;
            }
        }

        // Player 1 projectiles hitting offline bots
        for (int j = 0; j < (int)offlineBots.size(); j++) {
            BotEnemy* b = offlineBots[j];
            if (b->IsDead()) continue;
            Vector2 cc = { b->GetPosition().x, b->GetPosition().y - b->GetJumpHeight() };
            if (ProjectileHitsAABB(projPos, cc)) {
                bool wasDead = b->IsDead();
                b->TakeDamage(playerProjs[i]->GetDamage());
                playerProjs[i]->Deactivate();
                if (!wasDead && b->IsDead()) {
                    player->AddKill();
                    b->AddDeath();
                }
                break;
            }
        }
    }

    // Offline bot projectiles hitting player or other bots (Free For All)
    for (auto b : offlineBots) {
        if (b->IsMonster()) {
            // Monster melee: if within contact range of target, deal damage on cooldown
            Character* nearestTarget = nullptr;
            float minDist = 50.0f; // melee contact range

            if (!player->IsDead()) {
                float dx = player->GetPosition().x - b->GetPosition().x;
                float dy = player->GetPosition().y - b->GetPosition().y;
                float d = sqrtf(dx*dx + dy*dy);
                if (d < minDist) { minDist = d; nearestTarget = player; }
            }
            for (auto other : offlineBots) {
                if (other == b || other->IsDead()) continue;
                
                // Monsters don't attack other monsters
                if (other->IsMonster()) continue;

                float dx = other->GetPosition().x - b->GetPosition().x;
                float dy = other->GetPosition().y - b->GetPosition().y;
                float d = sqrtf(dx*dx + dy*dy);
                if (d < minDist) { minDist = d; nearestTarget = other; }
            }

            if (nearestTarget && b->GetShootCooldown() <= 0.0f) {
                bool wasDead = nearestTarget->IsDead();
                nearestTarget->TakeDamage(10.0f); // Nerfed monster damage from 20.0 to 10.0
                b->SetShootCooldown(0.8f); // melee attack rate
                if (!wasDead && nearestTarget->IsDead()) {
                    b->AddKill();
                    nearestTarget->AddDeath();
                    if (nearestTarget == player) player->AddDeath();
                }
            }
        } else {
            // Ranged bot projectiles
            for (auto& p : b->GetProjectiles()) {
                if (!p->IsActive()) continue;

                bool hit = false;
                if (!player->IsDead()) {
                    Vector2 playerCC = { player->GetPosition().x, player->GetPosition().y - player->GetJumpHeight() };
                    if (ProjectileHitsAABB(p->GetPosition(), playerCC)) {
                        bool wasAlive = !player->IsDead();
                        player->TakeDamage(p->GetDamage());
                        p->Deactivate();
                        hit = true;
                        if (wasAlive && player->IsDead()) {
                            b->AddKill();
                            player->AddDeath();
                        }
                    }
                }

                if (hit) continue;

                for (auto other : offlineBots) {
                    if (other == b || other->IsDead()) continue;
                    Vector2 otherCC = { other->GetPosition().x, other->GetPosition().y - other->GetJumpHeight() };
                    if (ProjectileHitsAABB(p->GetPosition(), otherCC)) {
                        bool wasAlive = !other->IsDead();
                        other->TakeDamage(p->GetDamage());
                        p->Deactivate();
                        if (wasAlive && other->IsDead()) {
                            b->AddKill();
                            other->AddDeath();
                        }
                        break;
                    }
                }
            }
        }
    }
}

bool GameplayScreen::Update(float deltaTime) {
    worldTime += deltaTime;

    if (IsKeyPressed(KEY_ESCAPE)) {
        return false;
    }

    // Convert mouse screen coords -> world coords using Camera2D
    Vector2 mouseScreen = GetMousePosition();
    // Scale mouse from actual screen to virtual screen
    float scaleX = (float)VIRTUAL_WIDTH  / (float)GetScreenWidth();
    float scaleY = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();
    Vector2 mouseVirtual = { mouseScreen.x * scaleX, mouseScreen.y * scaleY };
    // Convert virtual screen coords to world coords via camera
    Vector2 mouseWorld = GetScreenToWorld2D(mouseVirtual, camera);

    // Set player aim target in world coordinates
    player->SetAimTarget(mouseWorld);

    player->Update(deltaTime);
    ResolveRockCollisions(player);

    if (player2) {
        // Simple auto-aim for player 2
        Character* nearestEnemy = GetNearestEnemy(player2->GetPosition());
        if (nearestEnemy) {
            player2->SetAimTarget(nearestEnemy->GetPosition());
        }
        player2->Update(deltaTime);
        ResolveRockCollisions(player2);
    }

    // Update bots in offline mode
    if (currentMode == GameMode::OFFLINE) {
        for (auto b : offlineBots) {
            Character* target = GetNearestTargetForBot(b);
            if (target) {
                b->UpdateAI(deltaTime, target->GetPosition());
            } else {
                b->UpdateAI(deltaTime, b->GetPosition());
            }
            b->Update(deltaTime);
            ResolveRockCollisions(b);

            if (b->ShouldRespawn()) {
                Vector2 spawnPos = GetFarSpawnPoint();
                b->ResetHealth(100.0f);
                b->GetProjectiles().clear();
                b->SetPosition(spawnPos);
                b->ResetDeathTimer();
            }
        }

        // Offline Player Respawn logic
        if (player->IsDead() && IsKeyPressed(KEY_R)) {
            player->ResetHealth(100.0f);
            Vector2 spawnPos = GetFarSpawnPoint();
            player->SetPosition(spawnPos);
        }
    }

    // --- ONLINE NETWORKING ---
    if (currentMode == GameMode::ONLINE) {
        // Update remote players' animations
        for (auto r : remotePlayers) {
            r->Update(deltaTime);
        }

        netSendTimer += deltaTime;
        if (netSendTimer >= NET_SEND_RATE) {
            netSendTimer = 0.0f;

            PacketPlayerUpdate pkt;
            pkt.header.type   = PacketType::PLAYER_UPDATE;
            pkt.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
            pkt.position      = player->GetPosition();
            pkt.state         = (int)player->GetCurrentState();
            pkt.currentWeaponIndex = player->GetWeaponIndex();
            pkt.faceDirection = player->GetFaceDirection();
            pkt.health        = player->GetHealth();
            pkt.jumpHeight    = player->GetJumpHeight();
            pkt.jumpVelocity  = player->GetJumpVelocity();
            std::strncpy(pkt.username, NetworkManager::GetInstance().localUsername.c_str(), sizeof(pkt.username) - 1);
            pkt.username[sizeof(pkt.username) - 1] = '\0';
            pkt.charSkin      = NetworkManager::GetInstance().localSkinIndex;

            NetworkManager::GetInstance().SendPacket(&pkt, sizeof(pkt), false);
        }

        if (player->DidShoot()) {
            Vector2 aim = player->GetAimTarget();
            Vector2 hand = player->GetHandPosition();
            float dx = aim.x - hand.x;
            float dy = aim.y - hand.y;
            float len = sqrtf(dx*dx + dy*dy);

            PacketPlayerShoot shootPkt;
            shootPkt.header.type   = PacketType::PLAYER_SHOOT;
            shootPkt.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
            shootPkt.aimDir        = (len > 1.0f) ? Vector2{dx/len, dy/len} : Vector2{1.0f, 0.0f};
            NetworkManager::GetInstance().SendPacket(&shootPkt, sizeof(shootPkt), true);
        }

        // Respawn logic
        if (player->IsDead() && IsKeyPressed(KEY_R)) {
            player->ResetHealth(100.0f);
            Vector2 spawnPos = { WORLD_WIDTH / 2.0f + GetRandomValue(-200, 200), WORLD_HEIGHT / 2.0f + GetRandomValue(-200, 200) };
            player->SetPosition(spawnPos);

            PacketPlayerRespawn resPkt;
            resPkt.header.type = PacketType::PLAYER_RESPAWN;
            resPkt.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
            resPkt.spawnPosition = spawnPos;
            resPkt.charSkin = NetworkManager::GetInstance().localSkinIndex;
            NetworkManager::GetInstance().SendPacket(&resPkt, sizeof(resPkt), true);
        }

        // Poll incoming events (positions/shoots from others)
        PollNetworkEvents(deltaTime);
    }

    CheckCollisions();

    // Update camera to smoothly follow player 1
    Vector2 playerPos = player->GetPosition();
    camera.target.x += (playerPos.x - camera.target.x) * 5.0f * deltaTime;
    camera.target.y += (playerPos.y - camera.target.y) * 5.0f * deltaTime;

    // Clamp camera so it doesn't go outside world bounds
    float halfVW = VIRTUAL_WIDTH  / 2.0f;
    float halfVH = VIRTUAL_HEIGHT / 2.0f;
    camera.target.x = fclamp(camera.target.x, halfVW,  (float)WORLD_WIDTH  - halfVW);
    camera.target.y = fclamp(camera.target.y, halfVH,  (float)WORLD_HEIGHT - halfVH);

    return true;
}

void GameplayScreen::Draw(RenderTexture2D target) {
    BeginTextureMode(target);
    // Sky: vertical gradient from deep space blue at top to dark midnight at bottom
    ClearBackground({15, 15, 35, 255});
    // Draw sky gradient strips (above world, in screen space before Mode2D)
    // We'll do this in world space during BeginMode2D

    BeginMode2D(camera);

    // ---- LAYER 0: Deep background fill + sky gradient strips ----
    // Draw a subtle vertical sky gradient across the whole world
    for (int strip = 0; strip < WORLD_HEIGHT; strip += 80) {
        float t = (float)strip / (float)WORLD_HEIGHT;
        unsigned char r = (unsigned char)(15 + t * 10);
        unsigned char g = (unsigned char)(15 + t * 8);
        unsigned char b = (unsigned char)(35 + t * 15);
        DrawRectangle(0, strip, WORLD_WIDTH, 80, {r, g, b, 255});
    }

    // ---- LAYER 1: Tiled base ground (warm olive-green tint) ----
    if (bgTex1.id != 0 && bgTex1.width > 0) {
        int cols = (int)(WORLD_WIDTH  / bgTex1.width)  + 2;
        int rows = (int)(WORLD_HEIGHT / bgTex1.height) + 2;
        // Warm mossy green tint for ground
        Color groundTint = {72, 88, 55, 255};
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                DrawTexture(bgTex1, x * bgTex1.width, y * bgTex1.height, groundTint);
            }
        }
    }

    // ---- LAYER 2: Pebble/detail overlay (slightly darker, partial opacity) ----
    if (bgTex2.id != 0 && bgTex2.width > 0) {
        int cols = (int)(WORLD_WIDTH  / bgTex2.width)  + 2;
        int rows = (int)(WORLD_HEIGHT / bgTex2.height) + 2;
        Color detailTint = {55, 70, 40, 120};
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                DrawTexture(bgTex2, x * bgTex2.width, y * bgTex2.height, detailTint);
            }
        }
    }

    // ---- LAYER 3: Zone border highlights ----
    // Outer danger border glow (red zone edges)
    int borderW = 80;
    Color borderDanger = {180, 30, 30, 60};
    DrawRectangle(0, 0, WORLD_WIDTH, borderW, borderDanger);                       // top
    DrawRectangle(0, WORLD_HEIGHT - borderW, WORLD_WIDTH, borderW, borderDanger); // bottom
    DrawRectangle(0, 0, borderW, WORLD_HEIGHT, borderDanger);                     // left
    DrawRectangle(WORLD_WIDTH - borderW, 0, borderW, WORLD_HEIGHT, borderDanger); // right
    // Inner border line (bright orange-red outline)
    Color borderLine = {220, 80, 30, 100};
    int lineThk = 4;
    DrawRectangle(borderW, borderW, WORLD_WIDTH - borderW*2, lineThk, borderLine);
    DrawRectangle(borderW, WORLD_HEIGHT - borderW - lineThk, WORLD_WIDTH - borderW*2, lineThk, borderLine);
    DrawRectangle(borderW, borderW, lineThk, WORLD_HEIGHT - borderW*2, borderLine);
    DrawRectangle(WORLD_WIDTH - borderW - lineThk, borderW, lineThk, WORLD_HEIGHT - borderW*2, borderLine);

    // ---- LAYER 4: Animated ambient glow pools on ground ----
    struct GlowPool { float x, y, radius; Color col; };
    GlowPool glowPools[] = {
        { WORLD_WIDTH  * 0.5f,  WORLD_HEIGHT * 0.5f,  260.0f, {80,  200, 120,  0} }, // center green
        { WORLD_WIDTH  * 0.15f, WORLD_HEIGHT * 0.2f,  160.0f, {60,  100, 200,  0} }, // top-left blue
        { WORLD_WIDTH  * 0.85f, WORLD_HEIGHT * 0.2f,  160.0f, {200, 80,  60,   0} }, // top-right red
        { WORLD_WIDTH  * 0.15f, WORLD_HEIGHT * 0.8f,  160.0f, {200, 170, 40,   0} }, // bot-left gold
        { WORLD_WIDTH  * 0.85f, WORLD_HEIGHT * 0.8f,  160.0f, {120, 60,  200,  0} }, // bot-right purple
        { WORLD_WIDTH  * 0.5f,  WORLD_HEIGHT * 0.2f,  130.0f, {40,  180, 200,  0} }, // top-center cyan
        { WORLD_WIDTH  * 0.5f,  WORLD_HEIGHT * 0.8f,  130.0f, {200, 100, 160,  0} }, // bot-center pink
    };
    float pulse = 0.5f + 0.5f * sinf(worldTime * 1.2f);
    for (auto& gp : glowPools) {
        unsigned char alpha = (unsigned char)(25 + pulse * 30);
        Color c = { gp.col.r, gp.col.g, gp.col.b, alpha };
        // Outer glow (large, very transparent)
        DrawCircle((int)gp.x, (int)gp.y, gp.radius * 1.8f, { c.r, c.g, c.b, (unsigned char)(alpha / 3) });
        // Inner glow (smaller, more opaque)
        DrawCircle((int)gp.x, (int)gp.y, gp.radius, c);
        // Core bright center
        DrawCircle((int)gp.x, (int)gp.y, gp.radius * 0.3f, { c.r, c.g, c.b, (unsigned char)(alpha * 2 < 255 ? alpha * 2 : 255) });
    }

    // ---- LAYER 5: Ground patch accents (darker moss patches) ----
    if (bgDetail.id != 0) {
        struct Patch { float x, y, sc; Color col; };
        Patch patches[] = {
            { 600.0f,  800.0f,  2.5f, {50,  75,  35,  120} },
            { 1800.0f, 500.0f,  2.0f, {60,  90,  40,  100} },
            { 2500.0f, 1300.0f, 3.0f, {45,  65,  30,  110} },
            { 3200.0f, 900.0f,  2.2f, {55,  80,  38,  100} },
            { 1200.0f, 1700.0f, 2.8f, {50,  72,  33,  110} },
            { 800.0f,  300.0f,  1.8f, {65,  95,  45,  90}  },
            { 3600.0f, 400.0f,  2.0f, {48,  68,  32,  100} },
            { 400.0f,  1700.0f, 2.3f, {55,  78,  36,  105} },
        };
        for (auto& p : patches) {
            float w = bgDetail.width  * p.sc;
            float h = bgDetail.height * p.sc;
            Vector2 orig = { w / 2.0f, h / 2.0f };
            Rectangle src = { 0, 0, (float)bgDetail.width, (float)bgDetail.height };
            Rectangle dst = { p.x, p.y, w, h };
            DrawTexturePro(bgDetail, src, dst, orig, 0.0f, p.col);
        }
    }

    // ---- LAYER 6: Rock shadows (drawn before rocks) ----
    struct RockDraw {
        Vector2 pos;
        float scale;
        int type; // 0=rock3, 1=rock1, 2=rock2
        float rotation;
        Color tint;
    };
    // 20 rocks: first 10 use rockTex (rock3), next 5 rock1, next 5 rock2
    RockDraw rockDrawList[] = {
        // rock3 (angular cracked boulders) - tinted warm grey/brown
        { {400.0f,   500.0f},  0.55f, 0, -5.0f,   {200, 190, 175, 255} },
        { {900.0f,   900.0f},  0.75f, 0,  8.0f,   {185, 175, 160, 255} },
        { {1600.0f,  400.0f},  0.45f, 0, -12.0f,  {210, 195, 180, 255} },
        { {2200.0f,  1100.0f}, 0.65f, 0,  6.0f,   {195, 185, 168, 255} },
        { {2800.0f,  700.0f},  0.50f, 0, -8.0f,   {200, 188, 172, 255} },
        { {3400.0f,  1500.0f}, 0.70f, 0,  10.0f,  {190, 180, 165, 255} },
        { {1100.0f,  1600.0f}, 0.45f, 0, -15.0f,  {205, 192, 178, 255} },
        { {2600.0f,  350.0f},  0.55f, 0,  4.0f,   {195, 183, 168, 255} },
        { {700.0f,   1400.0f}, 0.60f, 0, -7.0f,   {200, 188, 172, 255} },
        { {3000.0f,  1700.0f}, 0.40f, 0,  12.0f,  {208, 196, 180, 255} },
        // rock1 (flat cracked slabs) - slightly cool blue-grey
        { {500.0f,  1200.0f},  0.50f, 1, -18.0f,  {175, 185, 195, 255} },
        { {1400.0f,  700.0f},  0.45f, 1,  14.0f,  {170, 180, 190, 255} },
        { {3200.0f,  500.0f},  0.60f, 1, -10.0f,  {180, 188, 200, 255} },
        { {1900.0f, 1800.0f},  0.55f, 1,  20.0f,  {172, 182, 192, 255} },
        { {2400.0f, 1400.0f},  0.40f, 1, -22.0f,  {178, 186, 198, 255} },
        // rock2 (rounded boulders) - warm earthy tan
        { {150.0f,   750.0f},  0.50f, 2,  5.0f,   {210, 200, 175, 255} },
        { {3700.0f, 1100.0f},  0.65f, 2, -9.0f,   {205, 195, 170, 255} },
        { {1300.0f, 1200.0f},  0.45f, 2,  16.0f,  {215, 205, 180, 255} },
        { {3100.0f, 1200.0f},  0.55f, 2, -14.0f,  {208, 198, 173, 255} },
        { {2000.0f,  600.0f},  0.40f, 2,  7.0f,   {212, 202, 178, 255} },
    };

    // Draw rock shadows first
    for (auto& rd : rockDrawList) {
        Texture2D tex = (rd.type == 1) ? rockTex1 : (rd.type == 2) ? rockTex2 : rockTex;
        if (tex.id == 0) continue;
        float w = tex.width  * rd.scale * 1.1f;
        float h = tex.height * rd.scale * 0.4f;
        Vector2 orig = { w / 2.0f, 0.0f };
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
        Rectangle dst = { rd.pos.x + 12, rd.pos.y + 18, w, h };
        DrawTexturePro(tex, src, dst, orig, rd.rotation * 0.5f, {0, 0, 0, 50});
    }

    // Draw rocks with tints and rotations
    for (auto& rd : rockDrawList) {
        Texture2D tex = (rd.type == 1) ? rockTex1 : (rd.type == 2) ? rockTex2 : rockTex;
        if (tex.id == 0) continue;
        float w = tex.width  * rd.scale;
        float h = tex.height * rd.scale;
        Vector2 orig = { w / 2.0f, h / 2.0f };
        Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
        Rectangle dst = { rd.pos.x, rd.pos.y, w, h };
        DrawTexturePro(tex, src, dst, orig, rd.rotation, rd.tint);
    }

    // Depth-sort all characters by Y position
    std::vector<Character*> allChars;
    allChars.push_back(player);
    if (player2) allChars.push_back(player2);
    for (auto r : remotePlayers) allChars.push_back(r);  // Online opponents
    for (auto b : offlineBots) allChars.push_back(b);    // Offline opponents

    // Sort characters by Y using a simple manual sort instead of a lambda to be beginner-friendly
    for (int i = 0; i < allChars.size(); i++) {
        for (int j = i + 1; j < allChars.size(); j++) {
            if (allChars[i]->GetPosition().y > allChars[j]->GetPosition().y) {
                Character* temp = allChars[i];
                allChars[i] = allChars[j];
                allChars[j] = temp;
            }
        }
    }

    for (auto c : allChars) c->Draw();

    EndMode2D();

    // HUD (drawn in screen space, not world space)

    // HUD: Animated Health bar (9-frame sprite animation) and Ammo
    const int barWidth = 500;
    const int barHeight = 100;
    int startX = 13;
    int startY = 0;

    // No clipping/shaping — draw it as-is so you can position it freely.
    if (headPortrait.id != 0 && headPortrait.width > 0) {
        int headSize = 85;                            // base size; tweak as needed
        int headX    = 30;                                // default x; adjust below
        int headY    = 5;                                // default y; adjust below
        Rectangle src = { 0.0f, 0.0f, (float)headPortrait.width, (float)headPortrait.height };
        Rectangle dst = { (float)headX, (float)headY, (float)headSize, (float)headSize };
        DrawTexturePro(headPortrait, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
    }

    // Pick frame based on health percentage (0.0 - 1.0)
    // Frame 1 (index 0) = full health, Frame 9 (index 8) = lowest health (reversed)
    {
        float healthPct = player->GetHealth() / 100.0f;
        if (healthPct < 0.0f) healthPct = 0.0f;
        if (healthPct > 1.0f) healthPct = 1.0f;
        int frameIndex = (int)((1.0f - healthPct) * 8.0f + 0.5f); // 0..8 (reversed: full HP = frame 1, low HP = frame 9)
        if (frameIndex < 0) frameIndex = 0;
        if (frameIndex > 8) frameIndex = 8;

        Texture2D& tex = healthBarFrames[frameIndex];
        if (tex.id != 0) {
            // Auto-detect the non-transparent content bounds (each frame may have different padding)
            // Lazy-init per frame, cache the source rect
            static Rectangle contentSrc[9] = {0};
            static bool contentSrcReady[9] = {false};
            int fi = frameIndex;
            if (!contentSrcReady[fi]) {
                // Load the image, find alpha bounds, then upload as GPU texture so we can free the CPU image
                Image img = LoadImage(
                    ("assets/HealthBar/" + std::to_string(fi + 1) + "_no_bg.png").c_str());
                if (img.data != nullptr) {
                    int minX = img.width, minY = img.height, maxX = -1, maxY = -1;
                    Color* px = LoadImageColors(img);
                    int n = img.width * img.height;
                    for (int k = 0; k < n; k++) {
                        if (px[k].a > 8) {
                            int x = k % img.width;
                            int y = k / img.width;
                            if (x < minX) minX = x;
                            if (y < minY) minY = y;
                            if (x > maxX) maxX = x;
                            if (y > maxY) maxY = y;
                        }
                    }
                    UnloadImageColors(px);
                    if (maxX >= 0) {
                        contentSrc[fi] = {
                            (float)minX, (float)minY,
                            (float)(maxX - minX + 1), (float)(maxY - minY + 1)
                        };
                    } else {
                        contentSrc[fi] = { 0, 0, (float)img.width, (float)img.height };
                    }
                    // Replace the cached GPU texture with the cropped one so further draws use tight bounds
                    UnloadTexture(tex);
                    Image crop = ImageFromImage(img, (Rectangle){
                        contentSrc[fi].x, contentSrc[fi].y,
                        contentSrc[fi].width, contentSrc[fi].height
                    });
                    UnloadImage(img);
                    tex = LoadTextureFromImage(crop);
                    UnloadImage(crop);
                    contentSrcReady[fi] = true;
                }
            }
            // Use the (possibly-cropped) texture's full size
            Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            Rectangle dst = { (float)startX, (float)startY, (float)barWidth, (float)barHeight };
            DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
        }
    }

    // ---- Dash bar (9-frame animation: snap to dash9 on dash, animate 9->1 while recharging) ----
    {
        float dashCooldown = player->GetDashCooldown();
        int frameIndex; // 0..8  ->  dash1.png..dash9.png
        if (dashCooldown > 0.0f) {
            // Drained (dash just happened) or recharging. dashCooldown goes 2.0 -> 0.
            // We want frame 9 (dash9.png, empty) at start of recharge, frame 1 (dash1.png, full) at end.
            float rechargePct = dashCooldown / 2.0f; // 1.0 just finished dash, 0.0 ready
            if (rechargePct > 1.0f) rechargePct = 1.0f;
            if (rechargePct < 0.0f) rechargePct = 0.0f;
            frameIndex = 8 - (int)(rechargePct * 8.0f + 0.5f); // 8 (dash9) .. 0 (dash1)
            if (frameIndex < 0) frameIndex = 0;
            if (frameIndex > 8) frameIndex = 8;
        } else {
            // Dash is ready -> show full frame (dash1)
            frameIndex = 0;
        }

        Texture2D& dtex = dashBarFrames[frameIndex];
        if (dtex.id != 0) {
            // Lazy-init per frame, cache the source rect (mirrors health bar logic)
            static Rectangle dashContentSrc[9] = {0};
            static bool dashContentSrcReady[9] = {false};
            int fi = frameIndex;
            if (!dashContentSrcReady[fi]) {
                Image img = LoadImage(
                    ("assets/DashBar/dash" + std::to_string(fi + 1) + ".png").c_str());
                if (img.data != nullptr) {
                    int minX = img.width, minY = img.height, maxX = -1, maxY = -1;
                    Color* px = LoadImageColors(img);
                    int n = img.width * img.height;
                    for (int k = 0; k < n; k++) {
                        if (px[k].a > 8) {
                            int x = k % img.width;
                            int y = k / img.width;
                            if (x < minX) minX = x;
                            if (y < minY) minY = y;
                            if (x > maxX) maxX = x;
                            if (y > maxY) maxY = y;
                        }
                    }
                    UnloadImageColors(px);
                    if (maxX >= 0) {
                        dashContentSrc[fi] = {
                            (float)minX, (float)minY,
                            (float)(maxX - minX + 1), (float)(maxY - minY + 1)
                        };
                    } else {
                        dashContentSrc[fi] = { 0, 0, (float)img.width, (float)img.height };
                    }
                    // Replace the cached GPU texture with the cropped one so further draws use tight bounds
                    UnloadTexture(dtex);
                    Image crop = ImageFromImage(img, (Rectangle){
                        dashContentSrc[fi].x, dashContentSrc[fi].y,
                        dashContentSrc[fi].width, dashContentSrc[fi].height
                    });
                    UnloadImage(img);
                    dtex = LoadTextureFromImage(crop);
                    UnloadImage(crop);
                    dashContentSrcReady[fi] = true;
                }
            }
            // Dash bar sits directly below the health bar (health bar is 100px tall at y=0)
            int dashStartX = 122;
            int dashStartY = 63 + 5; // 5px gap below the health bar
            const int dashBarWidth  = 300;
            const int dashBarHeight = 10;  // slimmer than the health bar so it reads as a secondary indicator
            Rectangle dSrc = { 0.0f, 0.0f, (float)dtex.width, (float)dtex.height };
            Rectangle dDst = { (float)dashStartX, (float)dashStartY,
                               (float)dashBarWidth, (float)dashBarHeight };
            DrawTexturePro(dtex, dSrc, dDst, {0.0f, 0.0f}, 0.0f, WHITE);
        }
    }

    if (player->IsDead() && (!player2 || player2->IsDead())) {
        DrawText("YOU'RE DEAD", VIRTUAL_WIDTH/2 - 120, VIRTUAL_HEIGHT/2 - 20, 48, RED);
        DrawText("Press R to Respawn", VIRTUAL_WIDTH/2 - 90, VIRTUAL_HEIGHT/2 + 40, 20, RAYWHITE);
    }

    // Scoreboard / Leaderboard
    if (IsKeyDown(KEY_TAB)) {
        if (currentMode == GameMode::ONLINE) {
            DrawRectangle(VIRTUAL_WIDTH/2 - 200, 100, 400, 300, {0, 0, 0, 200});
            DrawText("SCOREBOARD", VIRTUAL_WIDTH/2 - MeasureText("SCOREBOARD", 20)/2, 110, 20, WHITE);
            int y = 150;
            DrawText(TextFormat("%s : %d Kills / %d Deaths", NetworkManager::GetInstance().localUsername.c_str(), NetworkManager::GetInstance().localKills, NetworkManager::GetInstance().localDeaths), VIRTUAL_WIDTH/2 - 180, y, 16, GREEN);
            y += 25;
            for (auto rp : remotePlayers) {
                DrawText(TextFormat("%s : %d Kills / %d Deaths", rp->username.c_str(), rp->kills, rp->deaths), VIRTUAL_WIDTH/2 - 180, y, 16, RAYWHITE);
                y += 25;
            }
        } else if (currentMode == GameMode::OFFLINE) {
            // --- Offline Leaderboard ---
            // Build sorted list: player + bots
            struct LBEntry { std::string name; int kills; int deaths; bool isPlayer; bool isMonster; };
            std::vector<LBEntry> board;
            board.push_back({ player->GetName().empty() ? "You" : player->GetName(), player->GetKills(), player->GetDeaths(), true, false });
            for (auto b : offlineBots) {
                board.push_back({ b->GetName().empty() ? "Bot" : b->GetName(), b->GetKills(), b->GetDeaths(), false, b->IsMonster() });
            }
            // Sort by kills descending
            for (int i = 0; i < (int)board.size(); i++) {
                for (int j = i+1; j < (int)board.size(); j++) {
                    if (board[j].kills > board[i].kills) std::swap(board[i], board[j]);
                }
            }

            int boardW = 500;
            int boardH = 40 + (int)board.size() * 26 + 20;
            int boardX = VIRTUAL_WIDTH/2 - boardW/2;
            int boardY = 80;
            DrawRectangle(boardX, boardY, boardW, boardH, {10, 10, 30, 220});
            DrawRectangleLines(boardX, boardY, boardW, boardH, {100, 150, 255, 200});
            DrawText("LEADERBOARD [TAB]", boardX + boardW/2 - MeasureText("LEADERBOARD [TAB]", 18)/2, boardY + 8, 18, {255, 220, 60, 255});

            int ey = boardY + 38;
            for (int i = 0; i < (int)board.size(); i++) {
                Color monsterColor = { 255, 100, 100, 255 };
                Color entryColor = board[i].isPlayer ? GREEN : (board[i].isMonster ? monsterColor : RAYWHITE);
                const char* medal = (i == 0) ? "#1 " : (i == 1) ? "#2 " : (i == 2) ? "#3 " : "   ";
                const char* tag = board[i].isMonster ? " [MONSTER]" : (board[i].isPlayer ? " [YOU]" : " [BOT]");
                DrawText(TextFormat("%s%s%s  K:%d  D:%d", medal, board[i].name.c_str(), tag, board[i].kills, board[i].deaths),
                         boardX + 14, ey, 14, entryColor);
                ey += 26;
            }
        }
    }

    // Crosshair (custom cursor) - draw at mouse position in screen space
    {
        Texture2D crosshairTex = TextureManager::GetTexture(
            "assets/Free 2D Animated Vector Game Character Sprites/"
            "Free 2D Animated Vector Game Character Sprites/Extras/crosshair.png");
        if (crosshairTex.id != 0) {
            Vector2 mousePos = GetMousePosition();
            float scaleX2 = (float)VIRTUAL_WIDTH  / (float)GetScreenWidth();
            float scaleY2 = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();
            Vector2 mouseVirt = { mousePos.x * scaleX2, mousePos.y * scaleY2 };
            float cScale = 0.5f;
            float cW = crosshairTex.width  * cScale;
            float cH = crosshairTex.height * cScale;
            Rectangle cSrc = { 0, 0, (float)crosshairTex.width, (float)crosshairTex.height };
            Rectangle cDst = { mouseVirt.x, mouseVirt.y, cW, cH };
            Vector2 cOrigin = { cW / 2.0f, cH / 2.0f };
            DrawTexturePro(crosshairTex, cSrc, cDst, cOrigin, 0.0f, WHITE);
        } else {
            // Fallback: draw a simple crosshair with lines
            Vector2 mp = GetMousePosition();
            float scaleX2 = (float)VIRTUAL_WIDTH  / (float)GetScreenWidth();
            float scaleY2 = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();
            Vector2 mv = { mp.x * scaleX2, mp.y * scaleY2 };
            DrawLine((int)mv.x - 10, (int)mv.y, (int)mv.x + 10, (int)mv.y, WHITE);
            DrawLine((int)mv.x, (int)mv.y - 10, (int)mv.x, (int)mv.y + 10, WHITE);
        }
    }

    EndTextureMode();
}

// --- NETWORKING FUNCTIONS ---

const int DEFAULT_PLAYER_SKIN = 1;

// Find a RemotePlayer by authoritative playerID, or create one if it's new.
// Always keyed by the host-assigned playerID — never by ENet incomingPeerID.
RemotePlayer* GameplayScreen::FindOrCreateRemotePlayer(uint32_t playerID, int charSkin) {
    for (int i = 0; i < (int)remotePlayers.size(); i++) {
        if (remotePlayers[i]->peerID == playerID) {
            // Update the skin in case it changed (e.g., lobby -> game transition).
            return remotePlayers[i];
        }
    }
    // New player joined
    std::string charPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Full body animated characters/";
    // Ensure skin is 1-4
    if (charSkin < 1 || charSkin > 4) charSkin = 1;

    RemotePlayer* rp = new RemotePlayer(
        {WORLD_WIDTH / 2.0f + 100.0f, WORLD_HEIGHT / 2.0f},
        charPath + "Char " + std::to_string(charSkin) + "/with hands/"
    );
    rp->peerID = playerID;
    remotePlayers.push_back(rp);
    return rp;
}

// Remove a RemotePlayer by authoritative playerID (e.g., on disconnect).
void GameplayScreen::RemoveRemotePlayer(uint32_t playerID) {
    for (auto it = remotePlayers.begin(); it != remotePlayers.end(); ++it) {
        if ((*it)->peerID == playerID) {
            delete *it;
            remotePlayers.erase(it);
            return;
        }
    }
}

// Called each frame to process packets from the network
void GameplayScreen::PollNetworkEvents(float deltaTime) {
    NetworkManager::GetInstance().Update();
    std::vector<NetworkManager::NetworkEvent> events = NetworkManager::GetInstance().GetIncomingEvents();

    uint32_t myPlayerID = NetworkManager::GetInstance().GetLocalPlayerID();

    for (int i = 0; i < (int)events.size(); i++) {
        NetworkManager::NetworkEvent& ev = events[i];
        if (ev.data.size() < sizeof(PacketHeader)) continue;

        PacketHeader header;
        std::memcpy(&header, ev.data.data(), sizeof(PacketHeader));

        // Don't process our own packets sent back from the server.
        // While we are waiting for an ID assignment (client side) we must
        // accept packets from the host (playerID == 0) because our own ID
        // is not yet known.
        if (!NetworkManager::GetInstance().IsWaitingForAssignment() &&
            header.playerID == myPlayerID) {
            continue;
        }

        if (header.type == PacketType::PLAYER_UPDATE && ev.data.size() >= sizeof(PacketPlayerUpdate)) {
            PacketPlayerUpdate pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerUpdate));

            RemotePlayer* rp = FindOrCreateRemotePlayer(pkt.header.playerID, pkt.charSkin);
            rp->username = std::string(pkt.username);
            rp->ApplyNetworkUpdate(pkt.position, pkt.state, pkt.currentWeaponIndex, pkt.faceDirection, pkt.health, pkt.jumpHeight, pkt.jumpVelocity);

        } else if (header.type == PacketType::PLAYER_SHOOT && ev.data.size() >= sizeof(PacketPlayerShoot)) {
            PacketPlayerShoot pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerShoot));

            RemotePlayer* rp = FindOrCreateRemotePlayer(pkt.header.playerID, DEFAULT_PLAYER_SKIN);
            rp->SetFaceDirection(pkt.aimDir.x < 0 ? -1 : 1);
            rp->ShootInDirection(pkt.aimDir);

        } else if (header.type == PacketType::PLAYER_DAMAGE && ev.data.size() >= sizeof(PacketPlayerDamage)) {
            PacketPlayerDamage pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerDamage));

            // Is it us who was damaged?
            if (pkt.targetPlayerID == myPlayerID) {
                player->TakeDamage(pkt.damageAmount);
                if (player->IsDead()) {
                    NetworkManager::GetInstance().localDeaths++;
                    // Notify killer
                    PacketPlayerKilled killPkt;
                    killPkt.header.type = PacketType::PLAYER_KILLED;
                    killPkt.header.playerID = myPlayerID;
                    killPkt.killerPlayerID = pkt.header.playerID;
                    killPkt.victimPlayerID = pkt.targetPlayerID;
                    NetworkManager::GetInstance().SendPacket(&killPkt, sizeof(killPkt), true);
                }
            }
        } else if (header.type == PacketType::PLAYER_KILLED && ev.data.size() >= sizeof(PacketPlayerKilled)) {
            PacketPlayerKilled pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerKilled));
            if (pkt.killerPlayerID == myPlayerID) {
                NetworkManager::GetInstance().localKills++;
            } else {
                RemotePlayer* rp = FindOrCreateRemotePlayer(pkt.killerPlayerID, DEFAULT_PLAYER_SKIN);
                rp->kills++;
            }
            if (pkt.victimPlayerID != myPlayerID) {
                RemotePlayer* rp = FindOrCreateRemotePlayer(pkt.victimPlayerID, DEFAULT_PLAYER_SKIN);
                rp->deaths++;
            }
        } else if (header.type == PacketType::PLAYER_RESPAWN && ev.data.size() >= sizeof(PacketPlayerRespawn)) {
            PacketPlayerRespawn pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerRespawn));
            RemotePlayer* rp = FindOrCreateRemotePlayer(pkt.header.playerID, pkt.charSkin);
            rp->ResetHealth(100.0f);
            rp->SetPosition(pkt.spawnPosition);
        } else if (header.type == PacketType::PLAYER_DISCONNECT && ev.data.size() >= sizeof(PacketPlayerDisconnectHeader)) {
            PacketPlayerDisconnectHeader pkt;
            std::memcpy(&pkt, ev.data.data(), sizeof(PacketPlayerDisconnectHeader));
            RemoveRemotePlayer(pkt.header.playerID);
        }
    }
}

Vector2 GameplayScreen::GetFarSpawnPoint() {
    Vector2 spawnPoints[] = {
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
    int numSpawns = sizeof(spawnPoints) / sizeof(spawnPoints[0]);

    std::vector<Character*> activeChars;
    if (!player->IsDead()) activeChars.push_back(player);
    for (auto b : offlineBots) {
        if (!b->IsDead()) activeChars.push_back(b);
    }

    if (activeChars.empty()) {
        return spawnPoints[GetRandomValue(0, numSpawns - 1)];
    }

    Vector2 bestSpawn = spawnPoints[0];
    float maxMinDistSq = -1.0f;

    for (int i = 0; i < numSpawns; i++) {
        float minDistSq = 99999999.0f;
        for (auto c : activeChars) {
            float dx = c->GetPosition().x - spawnPoints[i].x;
            float dy = c->GetPosition().y - spawnPoints[i].y;
            float distSq = dx*dx + dy*dy;
            if (distSq < minDistSq) {
                minDistSq = distSq;
            }
        }
        if (minDistSq > maxMinDistSq) {
            maxMinDistSq = minDistSq;
            bestSpawn = spawnPoints[i];
        }
    }
    return bestSpawn;
}

Character* GameplayScreen::GetNearestTargetForBot(BotEnemy* b) {
    Character* nearest = nullptr;
    float minDist = 999999.0f;

    // Check player
    if (!player->IsDead()) {
        float dx = player->GetPosition().x - b->GetPosition().x;
        float dy = player->GetPosition().y - b->GetPosition().y;
        float dist = dx*dx + dy*dy;
        minDist = dist;
        nearest = player;
    }

    // Check other bots
    for (auto other : offlineBots) {
        if (other == b || other->IsDead()) continue;
        
        // Monsters don't target other monsters!
        if (b->IsMonster() && other->IsMonster()) continue;

        float dx = other->GetPosition().x - b->GetPosition().x;
        float dy = other->GetPosition().y - b->GetPosition().y;
        float dist = dx*dx + dy*dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = other;
        }
    }

    return nearest;
}
