#define RAYGUI_IMPLEMENTATION
#include <raylib.h>
#include "constants.h"
#include "utils/render_utils.h"
#include "screens/iscreen.h"
#include "screens/intro_screen.h"
#include "screens/main_level.h"
#include "screens/main_menu.h"
#include "screens/online_menu.h"
#include "screens/lobby_screen.h"
#include "network/network_manager.h"
#include "utils/texture_manager.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING); // Suppress INFO logs (texture loading spam = slow startup)
    InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "RJ - Intro");
    InitAudioDevice(); // Initialize audio device
    SetTargetFPS(60);

    NetworkManager::GetInstance().Initialize();

    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    IScreen* currentScreen = new FirstScreen();
    bool isFirstScreenActive = true;
    bool isMainMenuActive = false;
    bool isOnlineMenuActive = false;
    bool isLobbyActive = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        float deltaTime = GetFrameTime();
        
        // Update logic
        if (currentScreen) {
            if (!currentScreen->Update(deltaTime)) {
                if (isFirstScreenActive) {
                    delete currentScreen;
                    currentScreen = new MainMenuScreen();
                    isFirstScreenActive = false;
                    isMainMenuActive = true;
                } else if (isMainMenuActive) {
                    MainMenuScreen* menu = static_cast<MainMenuScreen*>(currentScreen);
                    GameMode mode = menu->GetSelectedMode();
                    delete currentScreen;
                    if (mode == GameMode::ONLINE) {
                        currentScreen = new OnlineMenuScreen();
                        isOnlineMenuActive = true;
                    } else {
                        currentScreen = new GameplayScreen(mode);
                    }
                    isMainMenuActive = false;
                } else if (isOnlineMenuActive) {
                    OnlineMenuScreen* menu = static_cast<OnlineMenuScreen*>(currentScreen);
                    if (menu->ShouldStartGame()) {
                        delete currentScreen;
                        currentScreen = new GameplayScreen(GameMode::ONLINE);
                        isOnlineMenuActive = false;
                    } else if (menu->ShouldGoToLobby()) {
                        // Go to lobby screen
                        delete currentScreen;
                        currentScreen = new LobbyScreen();
                        isOnlineMenuActive = false;
                        isLobbyActive = true;
                    } else {
                        // User backed out to main menu
                        delete currentScreen;
                        currentScreen = new MainMenuScreen();
                        isOnlineMenuActive = false;
                        isMainMenuActive = true;
                    }
                } else if (isLobbyActive) {
                    LobbyScreen* lobby = static_cast<LobbyScreen*>(currentScreen);
                    if (lobby->ShouldStartGame()) {
                        delete currentScreen;
                        currentScreen = new GameplayScreen(GameMode::ONLINE);
                        isLobbyActive = false;
                    } else {
                        delete currentScreen;
                        currentScreen = new MainMenuScreen();
                        isLobbyActive = false;
                        isMainMenuActive = true;
                    }
                } else {
                    delete currentScreen;
                    currentScreen = nullptr;
                    break;
                }
            }
        }
        
        // Draw logic
        if (currentScreen) {
            currentScreen->Draw(target);
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        DrawScaledToScreen(target);
        EndDrawing();
    }

    if (currentScreen != nullptr) {
        delete currentScreen;
    }

    UnloadRenderTexture(target);
    // TextureManager cleanup
    TextureManager::UnloadAll();
    NetworkManager::GetInstance().Shutdown();
    
    CloseAudioDevice(); // Close audio device
    CloseWindow();
    return 0;
}
