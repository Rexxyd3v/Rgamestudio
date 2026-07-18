#ifndef OFFLINE_MENU_H
#define OFFLINE_MENU_H

#include "iscreen.h"
#include "../ui/weapon_skin_preview.h"
#include "../ui/character_preview.h"
#include "menu_background.h"
#include <raylib.h>
#include <string>

class OfflineMenuScreen : public IScreen {
public:
    OfflineMenuScreen();
    ~OfflineMenuScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;

    bool ShouldStartGame() const { return startGame; }

private:
    Font menuFont;

    // UI state
    float fadeIn;
    float blurStrength;
    float caretBlink;

    // Tabs
    int currentLeftTab; // 0 = FIGHTER, 1 = LOADOUT
    float hoverTabFighter;
    float hoverTabLoadout;

    // Input
    int activeInput; // 0 = none, 1 = name field
    std::string username;

    // Selections
    int currentSkin; // 1-4
    int currentGunSkin; // 0-3 (global weapon skin ID, but note: we store per slot in WeaponSkinPreview)

    // UI areas
    Rectangle shell;
    Rectangle stageArea;
    Rectangle selectorArea;
    Rectangle gunPreviewArea;
    Rectangle gunSelectorArea;
    Rectangle nameField;
    Rectangle startBtn;

    // Background and previews
    MenuBackground* background;
    CharacterPreview* preview;
    WeaponSkinPreview* skinPreview;

    // Error message
    std::string connectError; // Reusing for general error messages

    // Hover states for buttons
    float hoverStart;
    float hoverBack;

    // Flag to indicate when the user has pressed start
    bool startGame;
};

#endif // OFFLINE_MENU_H


