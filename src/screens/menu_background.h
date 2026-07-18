#ifndef MENU_BACKGROUND_H
#define MENU_BACKGROUND_H

#include <raylib.h>
#include <vector>
#include "../entities/bot_enemy.h"

// MenuBackground
//
// Atmospheric backdrop for menu screens. Default style is a calm cinematic
// beach still (tiled map + silhouettes + slow camera drift + soft blur).
// LIVE_GAMEPLAY keeps the older bot-fight simulation for optional use.
class MenuBackground {
public:
    enum class BackdropStyle {
        LIVE_GAMEPLAY   = 0,
        CINEMATIC_BEACH = 1
    };

    MenuBackground(int localSkinIndex, BackdropStyle style = BackdropStyle::CINEMATIC_BEACH);

    ~MenuBackground();

    void Update(float deltaTime, float fadeInAlpha);

    // Draw onto `target` with alpha and blurStrength (0 = sharp, 1 = heavy blur).
    void Draw(RenderTexture2D target, float alpha, float blurStrength);

private:
    bool useCinematic = true;

    std::vector<BotEnemy*> bots;

    struct Rock {
        Vector2 position;
        float   scale;
        float   radius;
        float   rotation;
        Color   tint;
        int     type;
        Texture2D* tex;
        float   texW;
        float   texH;
        float   platformTop;
        Rectangle CollisionBounds() const;
        void Draw() const;
        float GetDepthY() const { return position.y + 8.0f; }
    };
    struct Tree {
        Vector2 position;
        float   scale;
        int     type;
        Texture2D* tex;
        float   texW;
        float   texH;
        float   trunkHeightPx;
        float   trunkWidthFrac;
        float   trunkHeight;
        float   trunkWidth;
        float   trunkOffsetX;
        Rectangle TrunkBounds() const;
        void Draw() const;
        float GetDepthY() const { return position.y + texH * scale * 0.5f; }
    };

    std::vector<Rock>  rocks;
    std::vector<Tree>  trees;

    Texture2D bgTex;
    Texture2D rockTex;
    Texture2D rockTex1;
    Texture2D rockTex2;
    Texture2D palmTree1;
    Texture2D palmTree2;

    Camera2D camera;

    RenderTexture2D sceneRT;
    RenderTexture2D blurRT1;
    RenderTexture2D blurRT2;

    float worldTime;

    Vector2 fightFocus;
    float   broadcastTimer;
    float   cameraZoom;
    float   cameraZoomTarget;
    float   cameraZoomTimer;

    // Cinematic drift
    Vector2 cinematicBase;
    float   driftPhase;

    void InitWorld();
    void InitEntities(int localSkinIndex);
    void TickWorld(float deltaTime);
    void TickCinematicCamera(float deltaTime);
    void DrawScene();
    Character* GetNearestTargetForBot(BotEnemy* b);
};

#endif // MENU_BACKGROUND_H
