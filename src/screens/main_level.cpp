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

GameplayScreen::GameplayScreen(GameMode mode) : currentMode(mode), spawnTimer(0.0f), netSendTimer(0.0f) {
    std::string bgPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Environment/";
    bgTex1 = TextureManager::GetTexture(bgPath + "ground_white.png");
    bgTex2 = TextureManager::GetTexture(bgPath + "ground2_white.png");
    rockTex = TextureManager::GetTexture(bgPath + "rock3.png");

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
        offlineBots.push_back(new BotEnemy({WORLD_WIDTH / 2.0f - 300.0f, WORLD_HEIGHT / 2.0f}, charPath + "Char 2/with hands/"));
        offlineBots.push_back(new BotEnemy({WORLD_WIDTH / 2.0f + 300.0f, WORLD_HEIGHT / 2.0f}, charPath + "Char 3/with hands/"));
        offlineBots.push_back(new BotEnemy({WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f + 300.0f}, charPath + "Char 4/with hands/"));
        offlineBots.push_back(new BotEnemy({WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f - 300.0f}, charPath + "Char 5/with hands/"));
    }

    // No AI companions in Online PvP mode
    // No AI companions needed - it's PvP!

    // Scatter rocks around the world
    rocks = {
        {{400.0f,  500.0f},  0.5f, 80.0f,  0.0f},
        {{900.0f,  900.0f},  0.7f, 110.0f, 0.0f},
        {{1600.0f, 400.0f},  0.4f, 65.0f,  0.0f},
        {{2200.0f, 1100.0f}, 0.6f, 95.0f,  0.0f},
        {{2800.0f, 700.0f},  0.5f, 80.0f,  0.0f},
        {{3400.0f, 1500.0f}, 0.7f, 110.0f, 0.0f},
        {{1100.0f, 1600.0f}, 0.4f, 65.0f,  0.0f},
        {{2600.0f, 350.0f},  0.5f, 80.0f,  0.0f},
        {{700.0f,  1400.0f}, 0.6f, 95.0f,  0.0f},
        {{3000.0f, 1700.0f}, 0.4f, 65.0f,  0.0f},
    };
    // Set platformTop: approximate top Y of each rock so characters can stand on it
    for (auto& r : rocks) {
        // rock texture center is at position; top surface is about radius above center
        r.platformTop = r.position.y - r.radius * 0.8f;
    }

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

Character* GameplayScreen::GetNearestPlayerOrCompanion(Vector2 pos) {
    Character* nearest = nullptr;
    float minDist = 999999.0f;

    // Check player
    if (!player->IsDead()) {
        float dx = player->GetPosition().x - pos.x;
        float dy = player->GetPosition().y - pos.y;
        float dist = dx*dx + dy*dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = player;
        }
    }

    if (player2 && !player2->IsDead()) {
        float dx = player2->GetPosition().x - pos.x;
        float dy = player2->GetPosition().y - pos.y;
        float dist = dx*dx + dy*dy;
        if (dist < minDist) {
            minDist = dist;
            nearest = player2;
        }
    }

    return nearest;
}

// Resolve a character being pushed away from rocks (circular collision)
void GameplayScreen::ResolveRockCollisions(Character* c) {
    if (c->IsDead()) return;

    Vector2 pos = c->GetPosition();
    float charRadius = 18.0f; // small footprint

    for (const auto& r : rocks) {
        float dx = pos.x - r.position.x;
        float dy = pos.y - r.position.y;
        float dist = sqrtf(dx*dx + dy*dy);
        float combinedRadius = charRadius + r.radius;

        if (dist < combinedRadius && dist > 0.1f) {
            // Check if the character is roughly above the rock (able to jump on top)
            bool fromAbove = (pos.y < r.position.y) && (fabsf(dx) < r.radius * 0.8f);

            if (fromAbove) {
                // Land on top of the rock
                pos.y = r.platformTop - charRadius;
                // If character is descending, stop vertical motion
                if (c->GetJumpVelocity() < 0.0f) {
                    c->SetJumpVelocity(0.0f);
                    c->SetJumpHeight(0.0f);
                }
            } else {
                // Push character outward (side collision)
                float nx = dx / dist;
                float ny = dy / dist;
                float overlap = combinedRadius - dist;
                pos.x += nx * overlap;
                pos.y += ny * overlap;
            }
        }
    }
    c->SetPosition(pos);
}

// Sprite is 2048x2048 drawn at scale 0.08. Sprite center pinned to (position.x, draw_y).
// Measured from idle sprite alpha bounds (all 4 chars): body occupies
// x:[756,1298] (w=~540), y:[1033,1782] (h=~750) of the sprite.
// World offsets relative to sprite center (draw_y = position.y - jumpHeight):
//   body top    = (1033 - 1024) * 0.08 = +0.7   (head ~at sprite center, no body above)
//   body bottom = (1782 - 1024) * 0.08 = +60.6 (feet below center)
//   body half-width = ((1298-756)/2) * 0.08 = ~21.7
// So the character body is BELOW the sprite's pinned center, not centered on it.
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
                dmgPkt.damageAmount   = 25.0f;
                NetworkManager::GetInstance().SendPacket(&dmgPkt, sizeof(dmgPkt), true);
                t->TakeDamage(25.0f);
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
                b->TakeDamage(25.0f);
                playerProjs[i]->Deactivate();
                break;
            }
        }
    }

    // Offline bot projectiles hitting player
    Vector2 playerCC = { player->GetPosition().x, player->GetPosition().y - player->GetJumpHeight() };
    for (auto b : offlineBots) {
        for (auto& p : b->GetProjectiles()) {
            if (!p->IsActive()) continue;
            if (!player->IsDead()) {
                if (ProjectileHitsAABB(p->GetPosition(), playerCC)) {
                    player->TakeDamage(10.0f);
                    p->Deactivate();
                }
            }
        }
    }
}

bool GameplayScreen::Update(float deltaTime) {
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
            b->UpdateAI(deltaTime, player->GetPosition());
            ResolveRockCollisions(b);
        }
    }

    // --- ONLINE NETWORKING ---
    if (currentMode == GameMode::ONLINE) {
        // Update remote players' animations
        for (auto r : remotePlayers) {
            r->Update(deltaTime);
        }

        // Send our position to others every NET_SEND_RATE seconds
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

        // Did we shoot?
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
    ClearBackground({30, 30, 50, 255});

    BeginMode2D(camera);

    // Draw tiled floor over the entire world
    if (bgTex1.id != 0 && bgTex1.width > 0) {
        int cols = (int)(WORLD_WIDTH  / bgTex1.width)  + 2;
        int rows = (int)(WORLD_HEIGHT / bgTex1.height) + 2;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                DrawTexture(bgTex1, x * bgTex1.width, y * bgTex1.height, {90, 90, 110, 255});
            }
        }
    }

    // Draw rocks
    if (rockTex.id != 0) {
        for (const auto& r : rocks) {
            // Center the texture on rock position
            float w = rockTex.width  * r.scale;
            float h = rockTex.height * r.scale;
            Vector2 origin = {w / 2.0f, h / 2.0f};
            Rectangle src = {0, 0, (float)rockTex.width, (float)rockTex.height};
            Rectangle dst = {r.position.x, r.position.y, w, h};
            DrawTexturePro(rockTex, src, dst, origin, 0.0f, WHITE);
        }
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
    if (currentMode == GameMode::ONLINE) {
        DrawText("WASD: Move | SPACE: Jump | LMB: Shoot | 1/2/3: Change Gun", 14, 14, 16, {200, 200, 200, 200});
        DrawText(TextFormat("Opponents Online: %d", (int)remotePlayers.size()), 14, 58, 14, {255, 200, 100, 220});
    } else {
        DrawText("WASD: Move | SPACE: Jump | Left Click: Shoot", 14, 14, 16, {200, 200, 200, 200});
    }

    // Health bar
    DrawRectangle(14, 38, 200, 14, {60, 60, 60, 200});
    if (!player->IsDead()) {
        DrawRectangle(14, 38, (int)(200 * player->GetHealth() / 100.0f), 14, {80, 220, 80, 230});
    }
    DrawText("HP", 16, 39, 12, WHITE);

    if (player->IsDead() && (!player2 || player2->IsDead())) {
        DrawText("GAME OVER", VIRTUAL_WIDTH/2 - 120, VIRTUAL_HEIGHT/2 - 20, 48, RED);
        if (currentMode == GameMode::ONLINE) {
            DrawText("Press R to Respawn", VIRTUAL_WIDTH/2 - 90, VIRTUAL_HEIGHT/2 + 40, 20, RAYWHITE);
        }
    }

    // Scoreboard
    if (currentMode == GameMode::ONLINE && IsKeyDown(KEY_TAB)) {
        DrawRectangle(VIRTUAL_WIDTH/2 - 200, 100, 400, 300, {0, 0, 0, 200});
        DrawText("SCOREBOARD", VIRTUAL_WIDTH/2 - MeasureText("SCOREBOARD", 20)/2, 110, 20, WHITE);
        
        int y = 150;
        DrawText(TextFormat("%s : %d Kills / %d Deaths", NetworkManager::GetInstance().localUsername.c_str(), NetworkManager::GetInstance().localKills, NetworkManager::GetInstance().localDeaths), VIRTUAL_WIDTH/2 - 180, y, 16, GREEN);
        y += 25;
        for (auto rp : remotePlayers) {
            DrawText(TextFormat("%s : %d Kills / %d Deaths", rp->username.c_str(), rp->kills, rp->deaths), VIRTUAL_WIDTH/2 - 180, y, 16, RAYWHITE);
            y += 25;
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
