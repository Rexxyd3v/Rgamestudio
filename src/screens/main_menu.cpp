#include "main_menu.h"

MainMenuScreen::MainMenuScreen() {
    // Try to load font, fallback to default if not found
    menuFont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 60, nullptr, 0);
    if (menuFont.texture.id == 0) {
        menuFont = GetFontDefault();
    }
    
    selectedMode = GameMode::NONE;
    
    float btnWidth = 400.0f;
    float btnHeight = 80.0f;
    float startX = VIRTUAL_WIDTH / 2.0f - btnWidth / 2.0f;
    
    btn1Player = { startX, 300.0f, btnWidth, btnHeight };
    btn2Player = { startX, 420.0f, btnWidth, btnHeight };
    
    hover1P = false;
    hover2P = false;
}

MainMenuScreen::~MainMenuScreen() {
    if (menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(menuFont);
    }
}

bool MainMenuScreen::Update(float deltaTime) {
    Vector2 mouseScreen = GetMousePosition();
    float scaleX = (float)VIRTUAL_WIDTH  / (float)GetScreenWidth();
    float scaleY = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();
    Vector2 mouseVirtual = { mouseScreen.x * scaleX, mouseScreen.y * scaleY };

    hover1P = CheckCollisionPointRec(mouseVirtual, btn1Player);
    hover2P = CheckCollisionPointRec(mouseVirtual, btn2Player);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hover1P) {
            selectedMode = GameMode::OFFLINE;
            return false; // Transition
        }
        if (hover2P) {
            selectedMode = GameMode::ONLINE;
            return false; // Transition
        }
    }
    
    return true; // Keep running
}

void MainMenuScreen::Draw(RenderTexture2D target) {
    BeginTextureMode(target);
    ClearBackground(BLACK);
    
    DrawTextEx(menuFont, "MAIN MENU", { VIRTUAL_WIDTH / 2.0f - MeasureTextEx(menuFont, "MAIN MENU", 80, 2).x / 2.0f, 100 }, 80, 2, WHITE);
    
    // Draw 1 Player Button
    DrawRectangleRec(btn1Player, hover1P ? DARKGRAY : GRAY);
    DrawRectangleLinesEx(btn1Player, 3, hover1P ? WHITE : LIGHTGRAY);
    Vector2 text1PSize = MeasureTextEx(menuFont, "OFFLINE MODE", 40, 2);
    DrawTextEx(menuFont, "OFFLINE MODE", { btn1Player.x + btn1Player.width/2 - text1PSize.x/2, btn1Player.y + btn1Player.height/2 - text1PSize.y/2 }, 40, 2, WHITE);
    
    // Draw 2 Player Button
    DrawRectangleRec(btn2Player, hover2P ? DARKGRAY : GRAY);
    DrawRectangleLinesEx(btn2Player, 3, hover2P ? WHITE : LIGHTGRAY);
    Vector2 text2PSize = MeasureTextEx(menuFont, "ONLINE MODE", 40, 2);
    DrawTextEx(menuFont, "ONLINE MODE", { btn2Player.x + btn2Player.width/2 - text2PSize.x/2, btn2Player.y + btn2Player.height/2 - text2PSize.y/2 }, 40, 2, WHITE);

    EndTextureMode();
}
