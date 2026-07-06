#include "lobby_screen.h"
#include "../network/network_manager.h"
#include "../constants.h"
#include <iostream>

LobbyScreen::LobbyScreen() :
    startGame(false),
    isHost(NetworkManager::GetInstance().IsHost()),
    playerName(NetworkManager::GetInstance().localUsername),
    playerSkin(NetworkManager::GetInstance().localSkinIndex) {
    if (isHost) {
        lanAddress = NetworkManager::GetInstance().GetLocalIPAddress() + ":" +
                     std::to_string(DEFAULT_GAME_PORT);
    }
}

LobbyScreen::~LobbyScreen() {
}

bool LobbyScreen::Update(float deltaTime) {
    // Update network manager to process incoming packets
    NetworkManager::GetInstance().Update();

    // Check if we're host and all players are ready

    bool canStart = false;
    if (isHost) {
        canStart = NetworkManager::GetInstance().AllPlayersReady();
    }

    // Handle input
    if (IsKeyPressed(KEY_ESCAPE)) {
        NetworkManager::GetInstance().Disconnect();
        return false;
    }

    if (isHost && IsKeyPressed(KEY_C) && !lanAddress.empty()) {
        SetClipboardText(lanAddress.c_str());
    }

    // Handle ready toggle (for both host and clients)
    if (IsKeyPressed(KEY_R)) {
        // Toggle ready state by getting current state from network manager
        bool currentReady = NetworkManager::GetInstance().IsLocalPlayerReady();
        NetworkManager::GetInstance().SetPlayerReady(!currentReady);
    }

    // Handle start game (host only)
    if (IsKeyPressed(KEY_SPACE) && isHost && canStart) {
        // Start the game
        NetworkManager::GetInstance().StartGame();
        startGame = true;
        return false; // Signal to change screens
    }

    // Check if game start signal was received
    auto events = NetworkManager::GetInstance().GetIncomingEvents();
    for (const auto& event : events) {
        if (event.data.size() >= sizeof(PacketType)) {
            PacketType packetType = static_cast<PacketType>(event.data[0]);
            if (packetType == PacketType::GAME_START) {
                // Game start signal received
                startGame = true;
                return false; // Signal to change screens
            }
        }
    }

    return true;
}

void LobbyScreen::Draw(RenderTexture2D target) {
    BeginTextureMode(target);
    ClearBackground({20, 20, 40, 255});

    // Title
    DrawText("LOBBY", VIRTUAL_WIDTH / 2 - MeasureText("LOBBY", 40) / 2, 50, 40, WHITE);

    // Player info
    std::string hostText = isHost ? "You are the Host" : "You are a Player";
    DrawText(hostText.c_str(), 20, 100, 20, isHost ? GOLD : LIGHTGRAY);

    // Instructions
    DrawText("Press R to toggle Ready status", 20, 130, 20, LIGHTGRAY);
    if (isHost) {
        DrawText("Press SPACE to Start Game (when all ready)", 20, 160, 20,
                NetworkManager::GetInstance().AllPlayersReady() ? GREEN : GRAY);

        DrawText("Share with friends:", 20, 190, 18, LIGHTGRAY);
        DrawText(("LAN: " + lanAddress).c_str(), 20, 212, 18, SKYBLUE);
        DrawText("playit.gg: share your tunnel address (host:port)", 20, 234, 16, GRAY);
        DrawText("Press C to copy LAN address to clipboard", 20, 254, 16, GRAY);
    }

    // Player list
    DrawPlayerList(20, isHost ? 280 : 200, VIRTUAL_WIDTH - 40, VIRTUAL_HEIGHT - (isHost ? 360 : 280));

    // Status messages
    if (isHost) {
        int readyCount = NetworkManager::GetInstance().GetReadyPlayerCount();
        int totalCount = NetworkManager::GetInstance().GetTotalPlayerCount();
        std::string statusText = "Ready: " + std::to_string(readyCount) + "/" + std::to_string(totalCount);
        DrawText(statusText.c_str(), VIRTUAL_WIDTH / 2 - MeasureText(statusText.c_str(), 20) / 2,
                VIRTUAL_HEIGHT - 30, 20,
                (readyCount == totalCount && totalCount > 0) ? GREEN : WHITE);
    } else {
        bool isLocalPlayerReady = NetworkManager::GetInstance().IsLocalPlayerReady();
        std::string readyText = isLocalPlayerReady ? "READY" : "NOT READY";
        Color readyColor = isLocalPlayerReady ? GREEN : RED;
        DrawText(("Status: " + readyText).c_str(), VIRTUAL_WIDTH / 2 - MeasureText(("Status: " + readyText).c_str(), 20) / 2,
                VIRTUAL_HEIGHT - 30, 20, readyColor);
    }

    EndTextureMode();
}

void LobbyScreen::DrawPlayerList(float startX, float startY, float width, float height) {
    // Draw background for player list
    DrawRectangle(static_cast<int>(startX), static_cast<int>(startY),
                  static_cast<int>(width), static_cast<int>(height),
                  Fade(DARKGRAY, 0.3f));

    // Draw header
    DrawText("PLAYERS", static_cast<int>(startX + 10), static_cast<int>(startY + 10), 20, WHITE);

    // Get player list from network manager
    const auto& players = NetworkManager::GetInstance().GetPlayers();

    // Draw each player
    float yOffset = 40;
    float lineHeight = 25;
    int drawnCount = 0;
    for (size_t i = 0; i < players.size(); i++) {
        const auto& player = players[i];

        // Skip placeholder entries that have not yet been resolved to a real name.
        // This prevents "Unknown" rows from showing up in the lobby.
        if (player.username.empty() || player.username == "Unknown") continue;

        bool isLocalPlayer = (player.peerID == NetworkManager::GetInstance().GetLocalPlayerID());
        bool isHostPlayer = (player.peerID == 0); // Host is ID 0 (authoritative)

        // Player info text
        std::string playerText =
            std::to_string(drawnCount + 1) + ". " +
            player.username +
            " (Skin: " + std::to_string(player.charSkin) + ") " +
            (isHostPlayer ? "[HOST] " : "") +
            (isLocalPlayer ? "[YOU] " : "") +
            (player.isReady ? "[READY]" : "[NOT READY]");

        Color nameColor = WHITE;
        if (isLocalPlayer) nameColor = SKYBLUE;
        else if (isHostPlayer) nameColor = GOLD;
        if (!player.isReady) nameColor = Fade(nameColor, 0.5f);

        DrawText(playerText.c_str(),
                 static_cast<int>(startX + 15),
                 static_cast<int>(startY + yOffset + drawnCount * lineHeight),
                 20, nameColor);
        drawnCount++;
    }

    // If no players, show placeholder
    if (drawnCount == 0) {
        DrawText("Waiting for players...",
                 static_cast<int>(startX + 10), static_cast<int>(startY + 40), 20, GRAY);
    }
}