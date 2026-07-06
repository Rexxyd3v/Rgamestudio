#include "online_menu.h"
#include "../network/network_manager.h"
#include "../constants.h"
#include <iostream>
#include "raylib.h"

namespace {
bool TryJoinRoom(const std::string& address, bool& goToLobby) {
    if (address.empty()) {
        return false;
    }
    if (NetworkManager::GetInstance().JoinRoom(address)) {
        goToLobby = true;
        return true;
    }
    return false;
}
} // namespace

OnlineMenuScreen::OnlineMenuScreen() :
    startGame(false),
    goToLobby(false),
    activeInput(0),
    joinAddress(""),
    username("Player"),
    currentSkin(1) {
}

OnlineMenuScreen::~OnlineMenuScreen() {
}

bool OnlineMenuScreen::Update(float deltaTime) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (activeInput != 0) {
            activeInput = 0;
        } else {
            return false; // Go back
        }
    }

    if (activeInput != 0) {
        std::string* activeStr = nullptr;
        if (activeInput == 1) activeStr = &username;
        else if (activeInput == 2) activeStr = &joinAddress;

        if (activeStr) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (activeStr->length() < 64)) {
                    *activeStr += (char)key;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && activeStr->length() > 0) {
                activeStr->pop_back();
            }

            if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
                const char* clipboardText = GetClipboardText();
                if (clipboardText) {
                    std::string clip(clipboardText);
                    for (char c : clip) {
                        if ((c >= 32) && (c <= 125) && (activeStr->length() < 64)) {
                            *activeStr += c;
                        }
                    }
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (activeInput == 2) {
                    NetworkManager::GetInstance().localUsername = username;
                    NetworkManager::GetInstance().localSkinIndex = currentSkin;
                    if (TryJoinRoom(joinAddress, goToLobby)) {
                        return false;
                    }
                    activeInput = 0;
                } else if (activeInput == 1) {
                    activeInput = 0;
                }
            }
        }
    } else {
        Vector2 mousePoint = GetMousePosition();
        float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
        float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
        mousePoint.x /= scaleX;
        mousePoint.y /= scaleY;

        Rectangle nameBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 200, 200, 40 };
        Rectangle skinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 140, 200, 40 };
        Rectangle hostBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 60, 200, 50 };
        Rectangle joinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 30, 200, 50 };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePoint, nameBtn)) {
                activeInput = 1;
            } else if (CheckCollisionPointRec(mousePoint, skinBtn)) {
                currentSkin++;
                if (currentSkin > 4) currentSkin = 1;
            } else if (CheckCollisionPointRec(mousePoint, hostBtn)) {
                NetworkManager::GetInstance().localUsername = username;
                NetworkManager::GetInstance().localSkinIndex = currentSkin;
                if (NetworkManager::GetInstance().HostRoom(DEFAULT_GAME_PORT)) {
                    goToLobby = true;
                    return false;
                }
            } else if (CheckCollisionPointRec(mousePoint, joinBtn)) {
                activeInput = 2;
            }
        }
    }

    return true;
}

void OnlineMenuScreen::Draw(RenderTexture2D target) {
    BeginTextureMode(target);
    ClearBackground({20, 20, 40, 255});

    DrawText("ONLINE MODE", VIRTUAL_WIDTH / 2 - MeasureText("ONLINE MODE", 40) / 2, 80, 40, WHITE);

    if (activeInput == 2) {
        DrawText("Enter server address:", VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 60, 20, LIGHTGRAY);
        DrawText("Examples: 192.168.1.5  or  abc.gl.at.ply.gg:54321", VIRTUAL_WIDTH / 2 - 250,
                 VIRTUAL_HEIGHT / 2 - 35, 16, GRAY);
        DrawRectangle(VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 10, 300, 40, RAYWHITE);
        DrawText(joinAddress.c_str(), VIRTUAL_WIDTH / 2 - 140, VIRTUAL_HEIGHT / 2, 20, DARKGRAY);
        DrawText("Press ENTER to connect, ESC to cancel.", VIRTUAL_WIDTH / 2 - 200, VIRTUAL_HEIGHT / 2 + 50, 20, GRAY);
    } else {
        Vector2 mousePoint = GetMousePosition();
        float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
        float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
        mousePoint.x /= scaleX;
        mousePoint.y /= scaleY;

        Rectangle nameBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 200, 200, 40 };
        DrawRectangleRec(nameBtn, (activeInput == 1) ? WHITE : (CheckCollisionPointRec(mousePoint, nameBtn) ? LIGHTGRAY : GRAY));
        DrawText(username.c_str(), nameBtn.x + 10, nameBtn.y + 10, 20, BLACK);
        DrawText("Name", nameBtn.x - 60, nameBtn.y + 10, 20, RAYWHITE);

        Rectangle skinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 140, 200, 40 };
        DrawRectangleRec(skinBtn, CheckCollisionPointRec(mousePoint, skinBtn) ? LIGHTGRAY : GRAY);
        std::string skinText = "Char " + std::to_string(currentSkin);
        DrawText(skinText.c_str(), skinBtn.x + 60, skinBtn.y + 10, 20, BLACK);
        DrawText("Skin", skinBtn.x - 60, skinBtn.y + 10, 20, RAYWHITE);

        Rectangle hostBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 60, 200, 50 };
        DrawRectangleRec(hostBtn, CheckCollisionPointRec(mousePoint, hostBtn) ? LIGHTGRAY : GRAY);
        DrawText("Create Room (Host)", hostBtn.x + 10, hostBtn.y + 15, 20, BLACK);

        Rectangle joinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 30, 200, 50 };
        DrawRectangleRec(joinBtn, CheckCollisionPointRec(mousePoint, joinBtn) ? LIGHTGRAY : GRAY);
        DrawText("Join Room", joinBtn.x + 50, joinBtn.y + 15, 20, BLACK);

        DrawText("Host: use playit.gg UDP tunnel -> port 7777", VIRTUAL_WIDTH / 2 - 180,
                 VIRTUAL_HEIGHT / 2 + 100, 16, GRAY);
        DrawText("Join: paste playit address (host:port) or LAN IP", VIRTUAL_WIDTH / 2 - 200,
                 VIRTUAL_HEIGHT / 2 + 120, 16, GRAY);
    }
    EndTextureMode();
}
