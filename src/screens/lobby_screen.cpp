#include "lobby_screen.h"
#include "../constants.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_theme.h"
#include "../ui/map_catalog.h"
#include <cstring>

LobbyScreen::LobbyScreen() :
    startGame(false),
    isHost(NetworkManager::GetInstance().IsHost()),
    playerName(NetworkManager::GetInstance().localUsername),
    playerSkin(NetworkManager::GetInstance().localSkinIndex),
    background(nullptr),
    preview(nullptr),
    fadeIn(0.0f),
    blurStrength(0.75f),
    hoverReady(0.0f),
    hoverStart(0.0f),
    hoverCopy(0.0f),
    hoverGlobalBack(0.0f) {

    menuFont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 60, nullptr, 0);
    if (menuFont.texture.id == 0) menuFont = GetFontDefault();

    if (isHost) {
        lanAddress = NetworkManager::GetInstance().GetLocalIPAddress() + ":" +
                     std::to_string(DEFAULT_GAME_PORT);
    }

    background = new MenuBackground(playerSkin, MenuBackground::BackdropStyle::CINEMATIC_BEACH);
    preview = new CharacterPreview();

    shell = { 50.0f, 70.0f, 1180.0f, 530.0f };
    rosterPanel = { 80.0f, 150.0f, 640.0f, 420.0f };
    readyBtn = { 780.0f, 310.0f, 390.0f, 50.0f };
    startBtn = { 780.0f, 368.0f, 390.0f, 50.0f };
    copyBtn  = { 780.0f, 462.0f, 390.0f, 42.0f };
}

LobbyScreen::~LobbyScreen() {
    delete preview;
    delete background;
    if (menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(menuFont);
    }
}


bool LobbyScreen::Update(float deltaTime) {
    fadeIn = Ui::Approach(fadeIn, 1.0f, 2.0f, deltaTime);
    blurStrength = Ui::Approach(blurStrength, 0.45f, 0.4f, deltaTime);

    if (background) background->Update(deltaTime, fadeIn);
    if (preview) preview->Update(deltaTime);

    NetworkManager::GetInstance().Update();

    bool canStart = isHost && NetworkManager::GetInstance().AllPlayersReady();

    if (IsKeyPressed(KEY_ESCAPE)) {
        NetworkManager::GetInstance().Disconnect();
        return false;
    }

    // Back button like in OnlineMenu (top-left)
    Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    bool hBack = CheckCollisionPointRec(Ui::RemapMouseToVirtual(), globalBackBtn);
    hoverGlobalBack = Ui::Approach(hoverGlobalBack, hBack ? 1.0f : 0.0f, 8.0f, deltaTime);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hBack) {
        NetworkManager::GetInstance().Disconnect();
        return false;
    }


    if (isHost && IsKeyPressed(KEY_C) && !lanAddress.empty()) {
        SetClipboardText(lanAddress.c_str());
    }

    if (IsKeyPressed(KEY_R)) {
        bool currentReady = NetworkManager::GetInstance().IsLocalPlayerReady();
        NetworkManager::GetInstance().SetPlayerReady(!currentReady);
    }

    if (IsKeyPressed(KEY_SPACE) && isHost && canStart) {
        NetworkManager::GetInstance().StartGame();
        startGame = true;
        return false;
    }

    Vector2 mouse = Ui::RemapMouseToVirtual();
    bool hReady = CheckCollisionPointRec(mouse, readyBtn);
    bool hStart = canStart && CheckCollisionPointRec(mouse, startBtn);
    bool hCopy  = isHost && CheckCollisionPointRec(mouse, copyBtn);

    hoverReady = Ui::Approach(hoverReady, hReady ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverStart = Ui::Approach(hoverStart, hStart ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverCopy  = Ui::Approach(hoverCopy,  hCopy  ? 1.0f : 0.0f, 8.0f, deltaTime);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hReady) {
            bool currentReady = NetworkManager::GetInstance().IsLocalPlayerReady();
            NetworkManager::GetInstance().SetPlayerReady(!currentReady);
        } else if (hStart) {
            NetworkManager::GetInstance().StartGame();
            startGame = true;
            return false;
        } else if (hCopy && !lanAddress.empty()) {
            SetClipboardText(lanAddress.c_str());
        }

        // Host Map Selection Input
        if (isHost) {
            const float gap = 12.0f;
            float cardW = (390.0f - gap * 2) / 3.0f;
            float cardH = 72.0f;
            float mapStartY = 70.0f + 108.0f; // shell.y + 108
            for (int i = 0; i < 3; ++i) {
                Rectangle card = { 780.0f + i * (cardW + gap), mapStartY, cardW, cardH };
                MapId id = static_cast<MapId>(i);
                const char* path = MapPath(id);
                bool locked = (path == nullptr || path[0] == '\0');
                if (!locked && CheckCollisionPointRec(mouse, card) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    NetworkManager::GetInstance().selectedMapId = i;
                    
                    // Broadcast PacketMapChanged
                    PacketMapChanged pkt{};
                    pkt.header.type = PacketType::MAP_CHANGED;
                    pkt.header.playerID = NetworkManager::GetInstance().GetLocalPlayerID();
                    pkt.mapId = i;
                    NetworkManager::GetInstance().SendPacket(&pkt, sizeof(pkt), true);
                }
            }
        }
    }

    auto events = NetworkManager::GetInstance().GetIncomingEvents();
    for (const auto& event : events) {
        if (event.data.size() >= sizeof(PacketType)) {
            PacketType packetType = static_cast<PacketType>(event.data[0]);
            if (packetType == PacketType::GAME_START && event.data.size() >= sizeof(PacketGameStart)) {
                PacketGameStart pkt;
                std::memcpy(&pkt, event.data.data(), sizeof(PacketGameStart));
                NetworkManager::GetInstance().selectedMapId = pkt.mapId;
                startGame = true;
                return false;
            } else if (packetType == PacketType::MAP_CHANGED && event.data.size() >= sizeof(PacketMapChanged)) {
                PacketMapChanged pkt;
                std::memcpy(&pkt, event.data.data(), sizeof(PacketMapChanged));
                NetworkManager::GetInstance().selectedMapId = pkt.mapId;
            }
        }
    }

    return true;
}

void LobbyScreen::DrawRoster() {
    DrawRectangleRec(rosterPanel, Fade(BLACK, 0.28f));
    DrawRectangleLinesEx(rosterPanel, 1.5f, Fade(UiTheme::PanelBorder(), 0.7f));

    Ui::DrawSectionLabel(menuFont, "PLAYERS", { rosterPanel.x + 20, rosterPanel.y + 16 }, 20.0f);

    const auto& players = NetworkManager::GetInstance().GetPlayers();
    float y = rosterPanel.y + 56.0f;
    float rowH = 72.0f;
    int drawn = 0;

    for (const auto& player : players) {
        if (player.username.empty() || player.username == "Unknown") continue;

        bool isLocal = (player.peerID == NetworkManager::GetInstance().GetLocalPlayerID());
        bool isHostPlayer = (player.peerID == 0);

        Rectangle row = { rosterPanel.x + 16, y, rosterPanel.width - 32, rowH - 10 };
        DrawRectangleRec(row, Fade(UiTheme::ButtonIdle(), isLocal ? 0.95f : 0.7f));
        DrawRectangleLinesEx(row, 1.0f, Fade(isLocal ? UiTheme::SkyAccent() : UiTheme::PanelBorder(), 0.5f));

        Rectangle head = { row.x + 12, row.y + 10, 48, 48 };
        if (preview) preview->DrawHead(player.charSkin, head);
        DrawRectangleLinesEx(head, 1.5f, Fade(UiTheme::AccentGold(), 0.4f));

        Color nameCol = isLocal ? UiTheme::SkyAccent() : (isHostPlayer ? UiTheme::AccentGold() : UiTheme::TextPrimary());
        DrawTextEx(menuFont, player.username.c_str(),
                   { row.x + 76, row.y + 14 }, 20.0f, 1.0f, nameCol);

        std::string meta = std::string("Char ") + std::to_string(player.charSkin);
        if (isHostPlayer) meta += "  -  HOST";
        if (isLocal) meta += "  -  YOU";
        DrawTextEx(menuFont, meta.c_str(),
                   { row.x + 76, row.y + 40 }, 12.0f, 1.0f, UiTheme::TextMuted());

        // Ready pill
        const char* readyLabel = player.isReady ? "READY" : "WAIT";
        Color pill = player.isReady ? UiTheme::ReadyGreen() : Fade(UiTheme::TextMuted(), 0.55f);
        Vector2 psz = MeasureTextEx(menuFont, readyLabel, 14.0f, 1.0f);
        Rectangle pillR = { row.x + row.width - psz.x - 28, row.y + row.height * 0.5f - 14, psz.x + 20, 28 };
        DrawRectangleRec(pillR, Fade(pill, 0.25f));
        DrawRectangleLinesEx(pillR, 1.5f, pill);
        DrawTextEx(menuFont, readyLabel,
                   { pillR.x + 10, pillR.y + 6 }, 14.0f, 1.0f, pill);

        y += rowH;
        drawn++;
        if (y + rowH > rosterPanel.y + rosterPanel.height) break;
    }

    if (drawn == 0) {
        DrawTextEx(menuFont, "Waiting for players...",
                   { rosterPanel.x + 24, rosterPanel.y + 80 }, 18.0f, 1.0f, UiTheme::TextMuted());
    }
}

void LobbyScreen::Draw(RenderTexture2D target) {
    if (background) {
        background->Draw(target, fadeIn, blurStrength);
    } else {
        BeginTextureMode(target);
        ClearBackground(BLACK);
        EndTextureMode();
    }

    BeginTextureMode(target);
    Ui::DrawVignette(0.65f);
    Ui::DrawGlassPanel(shell, 0.95f);

    Ui::DrawSectionLabel(menuFont, "LOBBY", { shell.x + 36, shell.y + 18 }, 28.0f);
    const char* role = isHost ? "You are the Host" : "You are a Player";
    DrawTextEx(menuFont, role,
               { shell.x + 36, shell.y + 52 }, 13.0f, 1.0f,
               isHost ? UiTheme::AccentGold() : UiTheme::TextMuted());

    DrawRoster();

    // Right actions
    Ui::DrawSectionLabel(menuFont, "ROOM", { 780, shell.y + 18 }, 20.0f);

    int readyCount = NetworkManager::GetInstance().GetReadyPlayerCount();
    int totalCount = NetworkManager::GetInstance().GetTotalPlayerCount();
    std::string status = "Ready  " + std::to_string(readyCount) + " / " + std::to_string(totalCount);
    DrawTextEx(menuFont, status.c_str(), { 780, shell.y + 52 }, 14.0f, 1.0f, UiTheme::TextMuted());

    // Map selection drawing
    Ui::DrawSectionLabel(menuFont, "MAP", { 780, shell.y + 82 }, 14.0f);
    {
        auto mapItems = GetMapGalleryItems();
        const float gap = 12.0f;
        float cardW = (390.0f - gap * 2) / 3.0f;
        float cardH = 72.0f;
        float mapStartY = shell.y + 108.0f;
        Vector2 mouse = Ui::RemapMouseToVirtual();

        for (int i = 0; i < 3; ++i) {
            Rectangle card = { 780.0f + i * (cardW + gap), mapStartY, cardW, cardH };
            bool selected = (NetworkManager::GetInstance().selectedMapId == i);
            bool locked = (i > 0);
            bool hovered = isHost && !locked && CheckCollisionPointRec(mouse, card);

            Color bg = selected ? Color{ 40, 36, 28, 230 } : Color{ 22, 24, 32, 200 };
            if (hovered) bg = Color{ 34, 36, 48, 220 };
            DrawRectangleRec(card, bg);

            Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
            float borderThick = selected ? 2.5f : 1.0f;
            DrawRectangleLinesEx(card, borderThick, Fade(border, selected ? 1.0f : 0.4f));

            if (!locked) {
                if (i < (int)mapItems.size()) {
                    Texture2D tex = mapItems[i].thumbnail;
                    if (tex.id != 0) {
                        float scale = cardW / (float)tex.width;
                        float w = tex.width * scale;
                        float h = tex.height * scale;
                        float y = card.y + (card.height - h) / 2.0f;
                        DrawTextureEx(tex, { card.x, y }, 0.0f, scale, WHITE);
                    }
                }
                DrawRectangleRec({ card.x, card.y + card.height - 20, card.width, 20 }, Fade(BLACK, 0.6f));
                const char* mapLabel = (i < (int)mapItems.size()) ? mapItems[i].label : "???";
                Vector2 labelSize = MeasureTextEx(menuFont, mapLabel, 10.0f, 1.0f);
                DrawTextEx(menuFont, mapLabel,
                           { card.x + card.width * 0.5f - labelSize.x * 0.5f, card.y + card.height - 15 },
                           10.0f, 1.0f, selected ? UiTheme::AccentGold() : UiTheme::TextPrimary());
            } else {
                DrawRectangleRec(card, Fade(RED, 0.15f));
                const char* lockText = "LOCKED";
                Vector2 labelSize = MeasureTextEx(menuFont, lockText, 10.0f, 1.0f);
                DrawTextEx(menuFont, lockText, 
                           { card.x + card.width * 0.5f - labelSize.x * 0.5f, card.y + card.height * 0.5f - 5.0f },
                           10.0f, 1.0f, UiTheme::TextMuted());
            }
        }
    }

    bool localReady = NetworkManager::GetInstance().IsLocalPlayerReady();
    bool canStart = isHost && NetworkManager::GetInstance().AllPlayersReady();

    Ui::DrawMenuButton(readyBtn, menuFont,
                       localReady ? "UNREADY  (R)" : "READY  (R)",
                       hoverReady, !localReady, true);

    if (isHost) {
        Ui::DrawMenuButton(startBtn, menuFont, "START MATCH  (SPACE)",
                           hoverStart, true, canStart);

        DrawTextEx(menuFont, "SHARE", { 780, startBtn.y + 60 }, 12.0f, 1.0f, UiTheme::TextMuted());
        Ui::DrawMenuButton(copyBtn, menuFont, "COPY LAN ADDRESS  (C)",
                           hoverCopy, false, !lanAddress.empty());

        if (!lanAddress.empty()) {
            DrawTextEx(menuFont, lanAddress.c_str(),
                       { 780, copyBtn.y + copyBtn.height + 6 }, 12.0f, 1.0f, UiTheme::SkyAccent());
        }
        DrawTextEx(menuFont, "Or share your playit.gg tunnel",
                   { 780, copyBtn.y + copyBtn.height + 24 }, 11.0f, 1.0f, UiTheme::TextMuted());
    } else {
        DrawTextEx(menuFont, localReady ? "Waiting for host to start..." : "Press Ready when you're set",
                   { 780, readyBtn.y + 60 }, 14.0f, 1.0f, UiTheme::TextMuted());
    }

    // Back button like in OnlineMenu (top-left)
    Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    Ui::DrawMenuButton(globalBackBtn, menuFont, "BACK", hoverGlobalBack, false, true);

    EndTextureMode();
}

