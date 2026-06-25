#include "online_menu.h"
#include "../network/network_manager.h"
#include "../constants.h"
#include <iostream>
#include <sstream>

OnlineMenuScreen::OnlineMenuScreen() :
    startGame(false),
    activeInput(0),
    ipAddress("127.0.0.1"),
    username("Player"),
    currentSkin(1),
    roomCode(""),
    isHosting(false) {
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
        // Handle typing for username, IP, or room code
        std::string* activeStr = nullptr;
        if (activeInput == 1) activeStr = &username;
        else if (activeInput == 2) activeStr = &ipAddress;
        else if (activeInput == 3) activeStr = &roomCode;

        if (activeStr) {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (activeStr->length() < 15)) {
                    *activeStr += (char)key;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && activeStr->length() > 0) {
                activeStr->pop_back();
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (activeInput == 2 && ipAddress.length() > 0) {
                    // We pressed enter on IP - attempt to join
                    NetworkManager::GetInstance().localUsername = username;
                    NetworkManager::GetInstance().localSkinIndex = currentSkin;
                    if (NetworkManager::GetInstance().JoinRoom(ipAddress, 7777)) {
                        // Go to lobby screen
                        goToLobby = true;
                        return false;
                    } else {
                        activeInput = 0; // Failed to join
                    }
                } else if (activeInput == 3 && roomCode.length() > 0) {
                    // We pressed enter on room code - attempt to join using code
                    NetworkManager::GetInstance().localUsername = username;
                    NetworkManager::GetInstance().localSkinIndex = currentSkin;
                    if (NetworkManager::GetInstance().JoinRoom(roomCode, 7777)) {
                        // Go to lobby screen
                        goToLobby = true;
                        return false;
                    } else {
                        activeInput = 0; // Failed to join
                    }
                } else if (activeInput == 1) {
                    activeInput = 0; // Done typing name
                }
            }
        }
    } else {
        // Buttons
        Vector2 mousePoint = GetMousePosition();
        float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
        float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
        mousePoint.x /= scaleX;
        mousePoint.y /= scaleY;

        Rectangle nameBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 220, 200, 40 };
        Rectangle skinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 160, 200, 40 };
        Rectangle hostBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 80, 200, 50 };
        Rectangle joinIpBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 10, 200, 50 };
        Rectangle joinCodeBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 80, 200, 50 };

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePoint, nameBtn)) {
                activeInput = 1;
            } else if (CheckCollisionPointRec(mousePoint, skinBtn)) {
                currentSkin++;
                if (currentSkin > 4) currentSkin = 1;
            } else if (CheckCollisionPointRec(mousePoint, hostBtn)) {
                NetworkManager::GetInstance().localUsername = username;
                NetworkManager::GetInstance().localSkinIndex = currentSkin;
                if (NetworkManager::GetInstance().HostRoom(7777)) {
                    // Instead of starting game directly, go to lobby screen
                    // We'll need to change the screen state in the main loop
                    // For now, let's set a flag to indicate we should go to lobby
                    // In a real implementation, we'd change the current screen here
                    return false; // Go back to main menu for now - we'll fix this properly
                }
            } else if (CheckCollisionPointRec(mousePoint, joinIpBtn)) {
                activeInput = 2; // IP input
            } else if (CheckCollisionPointRec(mousePoint, joinCodeBtn)) {
                activeInput = 3; // Room code input
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
        DrawText("Enter Host IP Address:", VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 40, 20, LIGHTGRAY);
        DrawRectangle(VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 10, 300, 40, RAYWHITE);
        DrawText(ipAddress.c_str(), VIRTUAL_WIDTH / 2 - 140, VIRTUAL_HEIGHT / 2, 20, DARKGRAY);
        DrawText("Press ENTER to connect, ESC to cancel.", VIRTUAL_WIDTH / 2 - 200, VIRTUAL_HEIGHT / 2 + 50, 20, GRAY);
    } else if (activeInput == 3) {
        DrawText("Enter Room Code:", VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 40, 20, LIGHTGRAY);
        DrawRectangle(VIRTUAL_WIDTH / 2 - 150, VIRTUAL_HEIGHT / 2 - 10, 300, 40, RAYWHITE);
        DrawText(roomCode.c_str(), VIRTUAL_WIDTH / 2 - 140, VIRTUAL_HEIGHT / 2, 20, DARKGRAY);
        DrawText("Press ENTER to connect, ESC to cancel.", VIRTUAL_WIDTH / 2 - 200, VIRTUAL_HEIGHT / 2 + 50, 20, GRAY);
    } else {
        Vector2 mousePoint = GetMousePosition();
        float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
        float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
        mousePoint.x /= scaleX;
        mousePoint.y /= scaleY;

        // Name Button
        Rectangle nameBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 220, 200, 40 };
        DrawRectangleRec(nameBtn, (activeInput == 1) ? WHITE : (CheckCollisionPointRec(mousePoint, nameBtn) ? LIGHTGRAY : GRAY));
        DrawText(username.c_str(), nameBtn.x + 10, nameBtn.y + 10, 20, BLACK);
        DrawText("Name", nameBtn.x - 60, nameBtn.y + 10, 20, RAYWHITE);

        // Skin Button
        Rectangle skinBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 160, 200, 40 };
        DrawRectangleRec(skinBtn, CheckCollisionPointRec(mousePoint, skinBtn) ? LIGHTGRAY : GRAY);
        std::string skinText = "Char " + std::to_string(currentSkin);
        DrawText(skinText.c_str(), skinBtn.x + 60, skinBtn.y + 10, 20, BLACK);
        DrawText("Skin", skinBtn.x - 60, skinBtn.y + 10, 20, RAYWHITE);

        // Host Button
        Rectangle hostBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f - 80, 200, 50 };
        DrawRectangleRec(hostBtn, CheckCollisionPointRec(mousePoint, hostBtn) ? LIGHTGRAY : GRAY);
        DrawText("Create Room (Host)", hostBtn.x + 10, hostBtn.y + 15, 20, BLACK);

        // Join via IP Button
        Rectangle joinIpBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 10, 200, 50 };
        DrawRectangleRec(joinIpBtn, CheckCollisionPointRec(mousePoint, joinIpBtn) ? LIGHTGRAY : GRAY);
        DrawText("Join Room (IP)", joinIpBtn.x + 10, joinIpBtn.y + 15, 20, BLACK);

        // Join via Code Button
        Rectangle joinCodeBtn = { VIRTUAL_WIDTH / 2.0f - 100, VIRTUAL_HEIGHT / 2.0f + 80, 200, 50 };
        DrawRectangleRec(joinCodeBtn, CheckCollisionPointRec(mousePoint, joinCodeBtn) ? LIGHTGRAY : GRAY);
        DrawText("Join Room (Code)", joinCodeBtn.x + 10, joinCodeBtn.y + 15, 20, BLACK);

        // Display room code if hosting
        if (isHosting && !roomCode.empty()) {
            DrawText("Your Room Code:", VIRTUAL_WIDTH / 2 - 100, VIRTUAL_HEIGHT / 2 + 150, 20, LIGHTGRAY);
            DrawRectangle(VIRTUAL_WIDTH / 2 - 100, VIRTUAL_HEIGHT / 2 + 170, 200, 40, RAYWHITE);
            DrawText(roomCode.c_str(), VIRTUAL_WIDTH / 2 - 90, VIRTUAL_HEIGHT / 2 + 180, 20, DARKGRAY);
            DrawText("Share this code with friends to join", VIRTUAL_WIDTH / 2 - 140, VIRTUAL_HEIGHT / 2 + 220, 15, GRAY);
        }
    }

    EndTextureMode();
}