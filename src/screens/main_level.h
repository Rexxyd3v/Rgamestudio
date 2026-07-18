#ifndef GAMEPLAY_SCREEN_H
#define GAMEPLAY_SCREEN_H

#include <raylib.h>
#include "iscreen.h"
#include "../entities/player.h"
#include "../entities/remote_player.h"
#include "../entities/bot_enemy.h"
#include <vector>
#include <memory>
#include <functional>
#include "../constants.h"

// World is 3x the virtual screen
#define WORLD_WIDTH  3900
#define WORLD_HEIGHT 2040

// Forward declaration for the Rock draw cache
class GameplayScreen;

// A renderable item in the depth-sorted list. Stores its feet-Y (so the
// std::sort comparator is trivial) and a draw callback. The screen pushes
// Player, RemotePlayer, BotEnemy, Rock and Tree instances into the same
// list and sorts once per frame.
struct RenderItem {
    float depthY;
    std::function<void()> draw;
};

struct Rock {
    Vector2 position;   // center in world coords (matches the existing draw call)
    float   scale;      // draw scale of the rock sprite
    float   radius;     // side-collision radius
    float   visualHeight;
    float   platformTop;  // cached platform top Y

    int   type;         // 0 = rock3, 1 = rock1, 2 = rock2 (selects rockTex/rockTex1/rockTex2)
    float rotation;     // draw rotation in degrees
    Color tint;         // draw tint
    float shadowOffset; // used by GetDepthY(): small positive Y offset for the contact point

    // Cached when first drawn so the texture pointer is safe to use. The
    // GameplayScreen constructor fills these.
    Texture2D* tex = nullptr;
    float  texW = 0.0f;  // texture width
    float  texH = 0.0f;  // texture height

    // Depth-sort key: the rock's "feet" — the bottom of the silhouette,
    // which is also the contact point with the ground.
    float GetDepthY() const {
        return position.y + shadowOffset;
    }

    // Approximate AABB for movement collision. Slightly inside the
    // visible silhouette so the player can brush past a rock without
    // getting stuck on a single pixel of overlap.
    // Tight AABB around the visual base of the rock.
    // position is the sprite CENTER (DrawTexturePro uses centered origin).
    // We cover the lower-center portion where the rock physically sits on ground.
    Rectangle CollisionBounds() const {
        float w = texW * scale * 0.35f;  // narrower than full sprite (rocks taper)
        float h = texH * scale * 0.10f;  // reduced height
        // Center the box slightly below the sprite center so it sits on the rock base
        float cx = position.x;
        float cy = position.y + texH * scale * 0.06f;
        return { cx - w * 0.5f, cy - h * 0.5f, w, h };
    }

    // Draw shadow + the tinted rock sprite. Lives here so the depth-sort
    // pass can just call Rock::Draw() on each item.
    void Draw() const;
};

struct Tree {
    Vector2 position;   // top-left of dst rect (matches the existing draw call)
    float   scale;      // draw scale
    int     type;       // 0 -> palmTree1, 1 -> palmTree2
    float   trunkHeightPx;  // how many source pixels at the bottom of the texture are trunk
    float   trunkWidthFrac; // trunk width as a fraction of the texture width

    // Cached
    Texture2D* tex = nullptr;
    float texW = 0.0f;
    float texH = 0.0f;
    float trunkHeight = 0.0f;  // world-space height from dst top to the trunk base
    float trunkWidth  = 0.0f;  // world-space width of the trunk
    float trunkOffsetX = 0.0f; // world-space X offset of the trunk within the dst rect

    // Depth-sort key: the base of the trunk in world space. The spec asks
    // for trunkHeight, which from a top-left dst anchor is the dst y + height.
    // Ground contact point = bottom of sprite (center + half sprite height).
    // Characters with feet ABOVE this sort behind; feet BELOW sort in front.
    // Depth-sort key: the base of the trunk in world space.
    // We set this to where the actual wooden trunk meets the dirt mound,
    // so characters standing on the dirt mound will sort in front of the tree.
    float GetDepthY() const {
        float halfSprH = texH * scale * 0.5f;
        return position.y + halfSprH - (trunkHeightPx * scale * 0.85f);
    }

    // Only the trunk collides — leaves are non-solid.
    // position is the sprite CENTER (draw call uses centered origin).
    // Trunk sits at the BOTTOM of the sprite.
    Rectangle TrunkBounds() const {
        // Shift the bottom of the collision box UP so it covers the thickest part
        // of the trunk just above the dirt mound.
        float trunkBot = GetDepthY(); // match collision bottom to depth pivot
        
        // Make the collision box extremely thin vertically (just a tiny sliver)
        float colHeight = 20.0f * scale; 
        float trunkTop = trunkBot - colHeight;
        
        // Also make it a bit narrower horizontally so you don't get stuck on the edges
        float narrowWidth = trunkWidth * 0.45f;
        float halfW       = narrowWidth * 0.5f;
        
        return { position.x - halfW, trunkTop, narrowWidth, colHeight };
    }

    // Draw the palm tree (no rotation, no tint).
    void Draw() const;
};

class GameplayScreen : public IScreen {
public:
    explicit GameplayScreen(GameMode mode);
    ~GameplayScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;

private:
    // Entities
    GameMode currentMode;
    Player* player;
    Player* player2;
    std::vector<BotEnemy*> offlineBots;       // Offline AI opponents
    std::vector<RemotePlayer*> remotePlayers; // Online opponents
    std::vector<Rock> rocks;
    std::vector<Tree> trees;

    Camera2D camera;

    Texture2D bgTex1;
    Texture2D bgTex2;
    Texture2D bgDetail; // sand/detail layer
    Texture2D rockTex;  // rock3
    Texture2D rockTex1; // rock1
    Texture2D rockTex2; // rock2
    Texture2D palmTree1; // palm tree type 1
    Texture2D palmTree2; // palm tree type 2
    Texture2D healthBarFrames[9];  // Animated health bar: index 0=frame1 (lowest), index 8=frame9 (full)
    Texture2D dashBarFrames[6];    // Animated dash bar:   index 0=dash1.png (full), index 5=dash6.png (empty)
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
    // Resolves tree trunk + rock collision for a character by undoing the
    // most recent position write on each axis independently. Called from
    // Update() after the character writes position.
    void  ResolveWorldCollision(Character* c);
    void  CheckCollisions();
    Character* GetNearestEnemy(Vector2 pos);
    Vector2 GetFarSpawnPoint();
    Character* GetNearestTargetForBot(BotEnemy* b);
};

#endif
