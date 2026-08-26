#include "lobby_screen.h"
#include "../constants.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_theme.h"
#include "../ui/map_catalog.h"
#include "../map_loader/MapRegistry.h"
#include "../map_loader/raytmx.h"
#include <cstring>
#include <algorithm>

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

    shell       = { 50.0f,  70.0f, 1180.0f, 530.0f };
    rosterPanel = {  80.0f, 150.0f,  640.0f, 420.0f };

    // Right-column layout (rightX=780, rightW=390, shell bottom=600)
    // Avoid top overlap: "Ready X/Y" is at shell.y+52 = 122.
    // y=134  MAP label
    // y=148  Map cycle selector (h=32) -> bottom 180
    // y=186  Map preview (h=120) -> bottom 306
    // y=318  MODE label
    // y=332  Mode cycle selector (h=32) -> bottom 364
    // y=376  Chip row 0 label
    // y=388  Chip row 0 (h=28)  -> bottom 416
    // y=420  Chip row 1 (h=28)  -> bottom 448
    // y=456  READY btn  (h=36)  -> bottom 492
    // y=498  START btn  (h=36)  -> bottom 534
    // y=540  COPY btn   (h=32)  -> bottom 572
    const float rightX  = 780.0f;
    const float rightW  = 390.0f;
    const float cardGap = 10.0f;
    const float arrowW  = 40.0f;

    // Map cycle selector
    const float mapRowY = 148.0f;
    const float mapRowH = 32.0f;
    prevMapBtn = { rightX,                     mapRowY, arrowW,               mapRowH };
    mapDisplay = { rightX + arrowW + 4.0f,     mapRowY, rightW - arrowW*2-8, mapRowH };
    nextMapBtn = { rightX + rightW - arrowW,   mapRowY, arrowW,               mapRowH };
    mapPreview = { rightX,                     186.0f,  rightW,               120.0f };
    hoverPrevMap = 0.0f;
    hoverNextMap = 0.0f;

    // Mode cycle selector
    const float modeRowY = 332.0f;
    const float modeRowH = 32.0f;
    prevModeBtn = { rightX,                     modeRowY, arrowW,               modeRowH };
    modeDisplay = { rightX + arrowW + 4.0f,     modeRowY, rightW - arrowW*2-8, modeRowH };
    nextModeBtn = { rightX + rightW - arrowW,   modeRowY, arrowW,               modeRowH };
    hoverPrevMode = 0.0f;
    hoverNextMode = 0.0f;

    const float chipH  = 28.0f;
    const float chipW3 = (rightW - cardGap * 2.0f) / 3.0f;
    const float chipW2 = (rightW - cardGap) / 2.0f;
    const float chipY0 = 388.0f;
    const float chipY1 = 420.0f;
    
    for (int i = 0; i < 3; ++i) {
        chipWin[i]   = { rightX + i * (chipW3 + cardGap), chipY0, chipW3, chipH };
        chipLimit[i] = { rightX + i * (chipW3 + cardGap), chipY1, chipW3, chipH };
        hoverChipWin[i]   = 0.0f;
        hoverChipLimit[i] = 0.0f;
    }
    for (int i = 0; i < 2; ++i) {
        chipGoal[i]      = { rightX + i * (chipW2 + cardGap), chipY0, chipW2, chipH };
        hoverChipGoal[i] = 0.0f;
    }

    readyBtn = { rightX, 456.0f, rightW, 36.0f };
    startBtn = { rightX, 498.0f, rightW, 36.0f };
    copyBtn  = { rightX, 540.0f, rightW, 32.0f };

    for (int i = 0; i < MAX_LOBBY_PLAYERS; ++i) {
        switchGR[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
        switchBL[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
        hoverSwitchGR[i] = 0.0f;
        hoverSwitchBL[i] = 0.0f;
    }

    cachedPreview = LoadRenderTexture((int)mapPreview.width, (int)mapPreview.height);
    cachedPreviewName = "";
}

LobbyScreen::~LobbyScreen() {
    UnloadRenderTexture(cachedPreview);
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

    // Map cycle selector hover animations
    bool hPrevMap = isHost && CheckCollisionPointRec(mouse, prevMapBtn);
    bool hNextMap = isHost && CheckCollisionPointRec(mouse, nextMapBtn);
    hoverPrevMap = Ui::Approach(hoverPrevMap, hPrevMap ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverNextMap = Ui::Approach(hoverNextMap, hNextMap ? 1.0f : 0.0f, 8.0f, deltaTime);

    // Mode cycle selector hover animations
    bool hPrev = isHost && CheckCollisionPointRec(mouse, prevModeBtn);
    bool hNext = isHost && CheckCollisionPointRec(mouse, nextModeBtn);
    hoverPrevMode = Ui::Approach(hoverPrevMode, hPrev ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverNextMode = Ui::Approach(hoverNextMode, hNext ? 1.0f : 0.0f, 8.0f, deltaTime);

    // Chip hover animations
    for (int i = 0; i < 3; ++i) {
        bool hovW = isHost && CheckCollisionPointRec(mouse, chipWin[i]);
        bool hovL = isHost && CheckCollisionPointRec(mouse, chipLimit[i]);
        hoverChipWin[i]   = Ui::Approach(hoverChipWin[i],   hovW ? 1.0f : 0.0f, 8.0f, deltaTime);
        hoverChipLimit[i] = Ui::Approach(hoverChipLimit[i], hovL ? 1.0f : 0.0f, 8.0f, deltaTime);
    }
    for (int i = 0; i < 2; ++i) {
        bool hovG = isHost && CheckCollisionPointRec(mouse, chipGoal[i]);
        hoverChipGoal[i] = Ui::Approach(hoverChipGoal[i], hovG ? 1.0f : 0.0f, 8.0f, deltaTime);
    }
    // Per-player team-switch pill hovers (only for rows the user can actually click).
    const auto& players = NetworkManager::GetInstance().GetPlayers();
    uint32_t myPid = NetworkManager::GetInstance().GetLocalPlayerID();
    for (int i = 0; i < (int)players.size() && i < MAX_LOBBY_PLAYERS; ++i) {
        if (players[i].username.empty() || players[i].username == "Unknown") continue;
        bool canSwitch = isHost || (players[i].peerID == myPid);
        bool hGR = canSwitch && CheckCollisionPointRec(mouse, switchGR[i]);
        bool hBL = canSwitch && CheckCollisionPointRec(mouse, switchBL[i]);
        hoverSwitchGR[i] = Ui::Approach(hoverSwitchGR[i], hGR ? 1.0f : 0.0f, 8.0f, deltaTime);
        hoverSwitchBL[i] = Ui::Approach(hoverSwitchBL[i], hBL ? 1.0f : 0.0f, 8.0f, deltaTime);
    }

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

        if (isHost) {
            // Map cycle selector clicks
            auto mapNames = MapRegistry::GetInstance().GetMapNames();
            if (!mapNames.empty()) {
                auto& nm = NetworkManager::GetInstance();
                int curIdx = 0;
                for (int i = 0; i < (int)mapNames.size(); ++i) {
                    if (mapNames[i] == nm.selectedMapName) {
                        curIdx = i;
                        break;
                    }
                }
                
                int newIdx = curIdx;
                if (CheckCollisionPointRec(mouse, prevMapBtn)) {
                    newIdx = (curIdx + mapNames.size() - 1) % mapNames.size();
                } else if (CheckCollisionPointRec(mouse, nextMapBtn)) {
                    newIdx = (curIdx + 1) % mapNames.size();
                }
                
                if (newIdx != curIdx) {
                    nm.selectedMapName = mapNames[newIdx];
                    PacketMapChanged pkt{};
                    pkt.header.type = PacketType::MAP_CHANGED;
                    pkt.header.playerID = nm.GetLocalPlayerID();
                    std::strncpy(pkt.mapName, mapNames[newIdx].c_str(), sizeof(pkt.mapName) - 1);
                    pkt.mapName[sizeof(pkt.mapName) - 1] = '\0';
                    nm.SendPacket(&pkt, sizeof(pkt), true);
                }
            }

            // Mode cycle selector clicks: < and > cycle through modes
            auto& nm = NetworkManager::GetInstance();
            int curMode = (int)nm.currentGameMode;
            if (CheckCollisionPointRec(mouse, prevModeBtn)) {
                int newMode = (curMode + 2) % 3; // go backwards
                nm.UpdateLobbySettings((OnlineGameMode)newMode, nm.timeLimit, nm.killLimit, nm.roundLimit);
            } else if (CheckCollisionPointRec(mouse, nextModeBtn)) {
                int newMode = (curMode + 1) % 3; // go forwards
                nm.UpdateLobbySettings((OnlineGameMode)newMode, nm.timeLimit, nm.killLimit, nm.roundLimit);
            }
            // Chip clicks — only the row matching the current mode is visible (others are
            // not drawn, but be defensive in case of overlap during transition).
            if (nm.currentGameMode == OnlineGameMode::ELIMINATION) {
                const int rounds[3] = { 6, 8, 14 };
                for (int i = 0; i < 3; ++i) {
                    if (CheckCollisionPointRec(mouse, chipWin[i]) && nm.roundLimit != rounds[i]) {
                        nm.UpdateLobbySettings(nm.currentGameMode, nm.timeLimit, nm.killLimit, rounds[i]);
                        break;
                    }
                }
            } else {
                bool isTimeGoal = (nm.timeLimit > 0);

                // Goal Type selection (first row: chipGoal)
                if (CheckCollisionPointRec(mouse, chipGoal[0]) && isTimeGoal) {
                    nm.UpdateLobbySettings(nm.currentGameMode, 0, 40, nm.roundLimit);
                } else if (CheckCollisionPointRec(mouse, chipGoal[1]) && !isTimeGoal) {
                    nm.UpdateLobbySettings(nm.currentGameMode, 300, 0, nm.roundLimit);
                }

                // Limit selection (second row: chipLimit)
                if (isTimeGoal) {
                    const int times[3] = { 300, 600, 900 };   // 5m / 10m / 15m in seconds
                    for (int i = 0; i < 3; ++i) {
                        if (CheckCollisionPointRec(mouse, chipLimit[i]) && nm.timeLimit != times[i]) {
                            nm.UpdateLobbySettings(nm.currentGameMode, times[i], 0, nm.roundLimit);
                            break;
                        }
                    }
                } else {
                    const int kills[3] = { 40, 60, 100 };
                    for (int i = 0; i < 3; ++i) {
                        if (CheckCollisionPointRec(mouse, chipLimit[i]) && nm.killLimit != kills[i]) {
                            nm.UpdateLobbySettings(nm.currentGameMode, 0, kills[i], nm.roundLimit);
                            break;
                        }
                    }
                }
            }

            // Team-switch pills (host can move anyone).
            const auto& players = nm.GetPlayers();
            uint32_t myPid = nm.GetLocalPlayerID();
            for (int i = 0; i < (int)players.size() && i < MAX_LOBBY_PLAYERS; ++i) {
                if (players[i].username.empty() || players[i].username == "Unknown") continue;
                if (CheckCollisionPointRec(mouse, switchGR[i])) {
                    nm.SetPlayerTeam(players[i].peerID, 1);
                } else if (CheckCollisionPointRec(mouse, switchBL[i])) {
                    nm.SetPlayerTeam(players[i].peerID, 2);
                }
            }
            (void)myPid; // silence unused warning if all players valid
        } else {
            // Non-host: clients may only move themselves. Find the local player index
            // by matching peerID. We mirror the host pill layout so the same rectangles
            // are reused for hover/draw.
            auto& nm = NetworkManager::GetInstance();
            const auto& players = nm.GetPlayers();
            uint32_t myPid = nm.GetLocalPlayerID();
            int localIdx = -1;
            for (int i = 0; i < (int)players.size() && i < MAX_LOBBY_PLAYERS; ++i) {
                if (players[i].peerID == myPid) { localIdx = i; break; }
            }
            if (localIdx >= 0) {
                if (CheckCollisionPointRec(mouse, switchGR[localIdx])) {
                    nm.RequestTeamChange(1);
                } else if (CheckCollisionPointRec(mouse, switchBL[localIdx])) {
                    nm.RequestTeamChange(2);
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
                NetworkManager::GetInstance().selectedMapName = pkt.mapName;
                startGame = true;
                return false;
            } else if (packetType == PacketType::MAP_CHANGED && event.data.size() >= sizeof(PacketMapChanged)) {
                PacketMapChanged pkt;
                std::memcpy(&pkt, event.data.data(), sizeof(PacketMapChanged));
                NetworkManager::GetInstance().selectedMapName = pkt.mapName;
            }
            // LOBBY_SETTINGS is already applied inside NetworkManager::Update(); the
            // next frame's redraw will pick up the new values.
        }
    }

    return true;
}

void LobbyScreen::DrawRoster() {
    DrawRectangleRec(rosterPanel, Fade(BLACK, 0.28f));
    DrawRectangleLinesEx(rosterPanel, 1.5f, Fade(UiTheme::PanelBorder(), 0.7f));

    const auto& players = NetworkManager::GetInstance().GetPlayers();
    OnlineGameMode mode = NetworkManager::GetInstance().currentGameMode;
    bool teamMode = (mode == OnlineGameMode::ELIMINATION || mode == OnlineGameMode::TEAM_DEATHMATCH);

    if (!teamMode) {
        // ---- Single-column FFA roster (legacy layout) ----
        Ui::DrawSectionLabel(menuFont, "PLAYERS", { rosterPanel.x + 20, rosterPanel.y + 16 }, 20.0f);
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
        // Clear any stale switch rectangles from a previous team-mode visit.
        for (int i = 0; i < MAX_LOBBY_PLAYERS; ++i) {
            switchGR[i] = switchBL[i] = { 0, 0, 0, 0 };
        }
        return;
    }

    // ---- Two-column team roster (Elimination / TDM) ----
    Ui::DrawSectionLabel(menuFont, "TEAMS", { rosterPanel.x + 20, rosterPanel.y + 16 }, 20.0f);

    const float colGap   = 12.0f;
    const float colW     = (rosterPanel.width - 32.0f - colGap) * 0.5f;
    const float colLeftX = rosterPanel.x + 16.0f;
    const float colRightX = colLeftX + colW + colGap;
    const float colTopY  = rosterPanel.y + 50.0f;
    const float rowH     = 60.0f;

    // Column headers
    DrawTextEx(menuFont, "GLOBAL RISK",
               { colLeftX + 8.0f, colTopY }, 14.0f, 1.0f, UiTheme::TeamBlue());
    DrawTextEx(menuFont, "BLACK LIST",
               { colRightX + 8.0f, colTopY }, 14.0f, 1.0f, UiTheme::TeamRed());
    // Underline each header
    DrawLineEx({ colLeftX,  colTopY + 22.0f }, { colLeftX + colW,  colTopY + 22.0f }, 2.0f, Fade(UiTheme::TeamBlue(), 0.8f));
    DrawLineEx({ colRightX, colTopY + 22.0f }, { colRightX + colW, colTopY + 22.0f }, 2.0f, Fade(UiTheme::TeamRed(), 0.8f));

    float yLeft  = colTopY + 30.0f;
    float yRight = colTopY + 30.0f;
    int drawnGR = 0, drawnBL = 0;

    uint32_t myPid = NetworkManager::GetInstance().GetLocalPlayerID();
    int rowIdx = 0;
    for (int p = 0; p < (int)players.size() && rowIdx < MAX_LOBBY_PLAYERS; ++p) {
        const auto& player = players[p];
        if (player.username.empty() || player.username == "Unknown") continue;

        bool isGR = (player.teamID == 1);
        bool isBL = (player.teamID == 2);
        bool isLocal    = (player.peerID == myPid);
        bool isHostPlayer = (player.peerID == 0);

        // Render unassigned players under GR by default so they don't disappear.
        bool leftColumn  = isGR || (!isBL);
        float colX = leftColumn ? colLeftX : colRightX;
        float& yRef = leftColumn ? yLeft : yRight;
        float yTop = yRef;
        float yMax = rosterPanel.y + rosterPanel.height - 4.0f;
        if (yTop + rowH > yMax) break; // out of room in this column

        Rectangle row = { colX + 4.0f, yTop, colW - 8.0f, rowH - 6.0f };
        Color borderCol = isGR ? UiTheme::TeamBlue()
                               : (isBL ? UiTheme::TeamRed() : UiTheme::PanelBorder());
        DrawRectangleRec(row, Fade(UiTheme::ButtonIdle(), isLocal ? 0.95f : 0.7f));
        DrawRectangleLinesEx(row, 1.0f, Fade(isLocal ? UiTheme::SkyAccent() : borderCol, 0.5f));

        Rectangle head = { row.x + 6, row.y + 6, 40, 40 };
        if (preview) preview->DrawHead(player.charSkin, head);
        DrawRectangleLinesEx(head, 1.5f, Fade(UiTheme::AccentGold(), 0.4f));

        Color nameCol = isLocal ? UiTheme::SkyAccent()
                                : (isHostPlayer ? UiTheme::AccentGold() : UiTheme::TextPrimary());
        // Truncate long names so they fit the narrower team column.
        std::string shown = player.username;
        if ((int)shown.size() > 10) shown = shown.substr(0, 10) + "..";
        DrawTextEx(menuFont, shown.c_str(),
                   { row.x + 52, row.y + 8 }, 16.0f, 1.0f, nameCol);
        std::string meta = std::string("C") + std::to_string(player.charSkin);
        if (isHostPlayer) meta += " HOST";
        if (isLocal) meta += " YOU";
        DrawTextEx(menuFont, meta.c_str(),
                   { row.x + 52, row.y + 30 }, 11.0f, 1.0f, UiTheme::TextMuted());

        // Ready pill
        const char* readyLabel = player.isReady ? "READY" : "WAIT";
        Color pill = player.isReady ? UiTheme::ReadyGreen() : Fade(UiTheme::TextMuted(), 0.55f);
        Vector2 psz = MeasureTextEx(menuFont, readyLabel, 11.0f, 1.0f);
        Rectangle pillR = { row.x + row.width - psz.x - 14, row.y + row.height * 0.5f - 11, psz.x + 12, 22 };
        DrawRectangleRec(pillR, Fade(pill, 0.25f));
        DrawRectangleLinesEx(pillR, 1.2f, pill);
        DrawTextEx(menuFont, readyLabel,
                   { pillR.x + 6, pillR.y + 4 }, 11.0f, 1.0f, pill);

        // Team-switch pills (host: anyone; client: only own row).
        bool canMove = isHost || isLocal;
        float pillW = 24.0f, pillH = 18.0f;
        float pillY = row.y + row.height - pillH - 4.0f;
        Rectangle grBtn = { row.x + row.width - 64.0f, pillY, pillW, pillH };
        Rectangle blBtn = { row.x + row.width - 36.0f, pillY, pillW, pillH };

        Color grIdle = Fade(UiTheme::TeamBlue(), 0.30f);
        Color blIdle = Fade(UiTheme::TeamRed(),  0.30f);
        Color grBorder = canMove ? UiTheme::TeamBlue() : Fade(UiTheme::TeamBlue(), 0.20f);
        Color blBorder = canMove ? UiTheme::TeamRed()  : Fade(UiTheme::TeamRed(),  0.20f);

        DrawRectangleRec(grBtn, Fade(grIdle, canMove ? 1.0f : 0.4f));
        DrawRectangleLinesEx(grBtn, 1.2f, Fade(grBorder, 0.6f + hoverSwitchGR[rowIdx] * 0.4f));
        DrawTextEx(menuFont, "GR", { grBtn.x + 5, grBtn.y + 3 }, 11.0f, 1.0f,
                   canMove ? UiTheme::TeamBlue() : Fade(UiTheme::TeamBlue(), 0.4f));
        DrawRectangleRec(blBtn, Fade(blIdle, canMove ? 1.0f : 0.4f));
        DrawRectangleLinesEx(blBtn, 1.2f, Fade(blBorder, 0.6f + hoverSwitchBL[rowIdx] * 0.4f));
        DrawTextEx(menuFont, "BL", { blBtn.x + 6, blBtn.y + 3 }, 11.0f, 1.0f,
                   canMove ? UiTheme::TeamRed() : Fade(UiTheme::TeamRed(), 0.4f));

        switchGR[rowIdx] = canMove ? grBtn : Rectangle{0,0,0,0};
        switchBL[rowIdx] = canMove ? blBtn : Rectangle{0,0,0,0};

        if (leftColumn) { yLeft += rowH; drawnGR++; }
        else            { yRight += rowH; drawnBL++; }
        rowIdx++;
    }
    // Hide any leftover pill rectangles from previous frames.
    for (int i = rowIdx; i < MAX_LOBBY_PLAYERS; ++i) {
        switchGR[i] = switchBL[i] = { 0, 0, 0, 0 };
    }
    if (drawnGR + drawnBL == 0) {
        DrawTextEx(menuFont, "Waiting for players...",
                   { rosterPanel.x + 24, rosterPanel.y + 80 }, 18.0f, 1.0f, UiTheme::TextMuted());
    }
}

void LobbyScreen::DrawModeSelector() {
    Ui::DrawSectionLabel(menuFont, "MODE", { 780.0f, modeDisplay.y - 16.0f }, 12.0f);

    const char* modeNames[3] = { "ELIMINATION", "FREE FOR ALL", "TEAM DM" };
    int activeMode = (int)NetworkManager::GetInstance().currentGameMode;
    const char* modeName = modeNames[activeMode];

    // Left arrow button [ < ]
    Color prevFill = Fade(UiTheme::ButtonIdle(), 1.0f);
    if (hoverPrevMode > 0.01f) prevFill = Fade(UiTheme::ButtonHover(), hoverPrevMode);
    DrawRectangleRec(prevModeBtn, prevFill);
    DrawRectangleLinesEx(prevModeBtn, 1.5f, Fade(UiTheme::PanelBorder(), 0.6f + hoverPrevMode * 0.4f));
    Vector2 centerPrev = { prevModeBtn.x + prevModeBtn.width/2, prevModeBtn.y + prevModeBtn.height/2 };
    DrawTriangle({centerPrev.x - 4, centerPrev.y}, {centerPrev.x + 4, centerPrev.y - 6}, {centerPrev.x + 4, centerPrev.y + 6}, isHost ? UiTheme::TextPrimary() : UiTheme::TextMuted());

    // Center display
    DrawRectangleRec(modeDisplay, Fade(UiTheme::AccentGold(), 0.15f));
    DrawRectangleLinesEx(modeDisplay, 2.0f, Fade(UiTheme::AccentGold(), 0.8f));
    // Gold left accent bar
    DrawRectangle((int)modeDisplay.x, (int)modeDisplay.y, 4, (int)modeDisplay.height, UiTheme::AccentGold());
    Vector2 nameSz = MeasureTextEx(menuFont, modeName, 16.0f, 1.0f);
    DrawTextEx(menuFont, modeName,
        { modeDisplay.x + modeDisplay.width * 0.5f - nameSz.x * 0.5f,
          modeDisplay.y + modeDisplay.height * 0.5f - nameSz.y * 0.5f },
        16.0f, 1.0f, UiTheme::AccentGold());

    // Right arrow button [ > ]
    Color nextFill = Fade(UiTheme::ButtonIdle(), 1.0f);
    if (hoverNextMode > 0.01f) nextFill = Fade(UiTheme::ButtonHover(), hoverNextMode);
    DrawRectangleRec(nextModeBtn, nextFill);
    DrawRectangleLinesEx(nextModeBtn, 1.5f, Fade(UiTheme::PanelBorder(), 0.6f + hoverNextMode * 0.4f));
    Vector2 centerNext = { nextModeBtn.x + nextModeBtn.width/2, nextModeBtn.y + nextModeBtn.height/2 };
    DrawTriangle({centerNext.x + 4, centerNext.y}, {centerNext.x - 4, centerNext.y + 6}, {centerNext.x - 4, centerNext.y - 6}, isHost ? UiTheme::TextPrimary() : UiTheme::TextMuted());
}

void LobbyScreen::DrawChipRows() {
    if (!isHost) return; // clients see values but cannot click
    OnlineGameMode mode = NetworkManager::GetInstance().currentGameMode;
    if (mode == OnlineGameMode::ELIMINATION) {
        Ui::DrawSectionLabel(menuFont, "WIN LIMIT (ROUNDS)",
                             { 780.0f, chipWin[0].y - 18.0f }, 12.0f);
        const char* labels[3] = { "6", "8", "14" };
        int active = (NetworkManager::GetInstance().roundLimit == 8) ? 1
                   : (NetworkManager::GetInstance().roundLimit == 14) ? 2 : 0;
        for (int i = 0; i < 3; ++i) {
            Rectangle b = chipWin[i];
            bool selected = (i == active);
            Color fill = selected
                ? Fade(UiTheme::AccentGold(), 0.30f + hoverChipWin[i] * 0.20f)
                : Color{
                    (unsigned char)(UiTheme::ButtonIdle().r + (UiTheme::ButtonHover().r - UiTheme::ButtonIdle().r) * hoverChipWin[i]),
                    (unsigned char)(UiTheme::ButtonIdle().g + (UiTheme::ButtonHover().g - UiTheme::ButtonIdle().g) * hoverChipWin[i]),
                    (unsigned char)(UiTheme::ButtonIdle().b + (UiTheme::ButtonHover().b - UiTheme::ButtonIdle().b) * hoverChipWin[i]),
                    (unsigned char)(UiTheme::ButtonIdle().a + (UiTheme::ButtonHover().a - UiTheme::ButtonIdle().a) * hoverChipWin[i])
                  };
            DrawRectangleRec(b, fill);
            Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
            float thick = selected ? 2.0f : 1.0f;
            DrawRectangleLinesEx(b, thick, Fade(border, 0.4f + hoverChipWin[i] * 0.6f));
            Vector2 sz = MeasureTextEx(menuFont, labels[i], 16.0f, 1.0f);
            DrawTextEx(menuFont, labels[i],
                       { b.x + b.width * 0.5f - sz.x * 0.5f,
                         b.y + b.height * 0.5f - sz.y * 0.5f },
                       16.0f, 1.0f, selected ? UiTheme::AccentGold() : UiTheme::TextPrimary());
        }
    } else {
        // FFA / TDM
        bool isTimeGoal = (NetworkManager::GetInstance().timeLimit > 0);

        Ui::DrawSectionLabel(menuFont, "GOAL TYPE",
                             { 780.0f, chipGoal[0].y - 18.0f }, 12.0f);
        const char* goalLabels[2] = { "KILLS", "TIME" };
        int activeGoal = isTimeGoal ? 1 : 0;
        for (int i = 0; i < 2; ++i) {
            Rectangle b = chipGoal[i];
            bool selected = (i == activeGoal);
            Color fill = selected
                ? Fade(UiTheme::AccentGold(), 0.30f + hoverChipGoal[i] * 0.20f)
                : Color{
                    (unsigned char)(UiTheme::ButtonIdle().r + (UiTheme::ButtonHover().r - UiTheme::ButtonIdle().r) * hoverChipGoal[i]),
                    (unsigned char)(UiTheme::ButtonIdle().g + (UiTheme::ButtonHover().g - UiTheme::ButtonIdle().g) * hoverChipGoal[i]),
                    (unsigned char)(UiTheme::ButtonIdle().b + (UiTheme::ButtonHover().b - UiTheme::ButtonIdle().b) * hoverChipGoal[i]),
                    (unsigned char)(UiTheme::ButtonIdle().a + (UiTheme::ButtonHover().a - UiTheme::ButtonIdle().a) * hoverChipGoal[i])
                  };
            DrawRectangleRec(b, fill);
            Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
            float thick = selected ? 2.0f : 1.0f;
            DrawRectangleLinesEx(b, thick, Fade(border, 0.4f + hoverChipGoal[i] * 0.6f));
            Vector2 sz = MeasureTextEx(menuFont, goalLabels[i], 16.0f, 1.0f);
            DrawTextEx(menuFont, goalLabels[i],
                       { b.x + b.width * 0.5f - sz.x * 0.5f,
                         b.y + b.height * 0.5f - sz.y * 0.5f },
                       16.0f, 1.0f, selected ? UiTheme::AccentGold() : UiTheme::TextPrimary());
        }

        if (isTimeGoal) {
            Ui::DrawSectionLabel(menuFont, "TIME LIMIT",
                                 { 780.0f, chipLimit[0].y - 18.0f }, 12.0f);
            const char* labels[3] = { "5m", "10m", "15m" };
            int cur = NetworkManager::GetInstance().timeLimit;
            int active = (cur == 300) ? 0 : (cur == 600) ? 1 : 2;
            for (int i = 0; i < 3; ++i) {
                Rectangle b = chipLimit[i];
                bool selected = (i == active);
                Color fill = selected
                    ? Fade(UiTheme::AccentGold(), 0.30f + hoverChipLimit[i] * 0.20f)
                    : Color{
                        (unsigned char)(UiTheme::ButtonIdle().r + (UiTheme::ButtonHover().r - UiTheme::ButtonIdle().r) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().g + (UiTheme::ButtonHover().g - UiTheme::ButtonIdle().g) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().b + (UiTheme::ButtonHover().b - UiTheme::ButtonIdle().b) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().a + (UiTheme::ButtonHover().a - UiTheme::ButtonIdle().a) * hoverChipLimit[i])
                      };
                DrawRectangleRec(b, fill);
                Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
                float thick = selected ? 2.0f : 1.0f;
                DrawRectangleLinesEx(b, thick, Fade(border, 0.4f + hoverChipLimit[i] * 0.6f));
                Vector2 sz = MeasureTextEx(menuFont, labels[i], 16.0f, 1.0f);
                DrawTextEx(menuFont, labels[i],
                           { b.x + b.width * 0.5f - sz.x * 0.5f,
                             b.y + b.height * 0.5f - sz.y * 0.5f },
                           16.0f, 1.0f, selected ? UiTheme::AccentGold() : UiTheme::TextPrimary());
            }
        } else {
            Ui::DrawSectionLabel(menuFont, "KILL LIMIT",
                                 { 780.0f, chipLimit[0].y - 18.0f }, 12.0f);
            const char* klabels[3] = { "40", "60", "100" };
            int curK = NetworkManager::GetInstance().killLimit;
            int activeK = (curK == 40) ? 0 : (curK == 60) ? 1 : 2;
            for (int i = 0; i < 3; ++i) {
                Rectangle b = chipLimit[i];
                bool selected = (i == activeK);
                Color fill = selected
                    ? Fade(UiTheme::AccentGold(), 0.30f + hoverChipLimit[i] * 0.20f)
                    : Color{
                        (unsigned char)(UiTheme::ButtonIdle().r + (UiTheme::ButtonHover().r - UiTheme::ButtonIdle().r) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().g + (UiTheme::ButtonHover().g - UiTheme::ButtonIdle().g) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().b + (UiTheme::ButtonHover().b - UiTheme::ButtonIdle().b) * hoverChipLimit[i]),
                        (unsigned char)(UiTheme::ButtonIdle().a + (UiTheme::ButtonHover().a - UiTheme::ButtonIdle().a) * hoverChipLimit[i])
                      };
                DrawRectangleRec(b, fill);
                Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
                float thick = selected ? 2.0f : 1.0f;
                DrawRectangleLinesEx(b, thick, Fade(border, 0.4f + hoverChipLimit[i] * 0.6f));
                Vector2 sz = MeasureTextEx(menuFont, klabels[i], 16.0f, 1.0f);
                DrawTextEx(menuFont, klabels[i],
                           { b.x + b.width * 0.5f - sz.x * 0.5f,
                             b.y + b.height * 0.5f - sz.y * 0.5f },
                           16.0f, 1.0f, selected ? UiTheme::AccentGold() : UiTheme::TextPrimary());
            }
        }
    }
}

void LobbyScreen::DrawMapCards() { /* map drawing moved inline in Draw() */ }

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
    Ui::DrawSectionLabel(menuFont, "MAP", { 780, mapDisplay.y - 16.0f }, 12.0f);
    {
        auto mapNames = MapRegistry::GetInstance().GetMapNames();
        std::string currentMap = NetworkManager::GetInstance().selectedMapName;
        
        // Prev button
        Color prevFill = Fade(UiTheme::ButtonIdle(), 1.0f);
        if (hoverPrevMap > 0.01f) prevFill = Fade(UiTheme::ButtonHover(), hoverPrevMap);
        DrawRectangleRec(prevMapBtn, prevFill);
        DrawRectangleLinesEx(prevMapBtn, 1.5f, Fade(UiTheme::PanelBorder(), 0.6f + hoverPrevMap * 0.4f));
        Vector2 centerPrev = { prevMapBtn.x + prevMapBtn.width/2, prevMapBtn.y + prevMapBtn.height/2 };
        DrawTriangle({centerPrev.x - 4, centerPrev.y}, {centerPrev.x + 4, centerPrev.y - 6}, {centerPrev.x + 4, centerPrev.y + 6}, isHost ? UiTheme::TextPrimary() : UiTheme::TextMuted());

        // Center display
        DrawRectangleRec(mapDisplay, Fade(UiTheme::ButtonIdle(), 0.15f));
        DrawRectangleLinesEx(mapDisplay, 2.0f, Fade(UiTheme::PanelBorder(), 0.8f));
        Vector2 nameSz = MeasureTextEx(menuFont, currentMap.c_str(), 14.0f, 1.0f);
        DrawTextEx(menuFont, currentMap.c_str(),
            { mapDisplay.x + mapDisplay.width * 0.5f - nameSz.x * 0.5f,
              mapDisplay.y + mapDisplay.height * 0.5f - nameSz.y * 0.5f },
            14.0f, 1.0f, UiTheme::TextPrimary());

        // Next button
        Color nextFill = Fade(UiTheme::ButtonIdle(), 1.0f);
        if (hoverNextMap > 0.01f) nextFill = Fade(UiTheme::ButtonHover(), hoverNextMap);
        DrawRectangleRec(nextMapBtn, nextFill);
        DrawRectangleLinesEx(nextMapBtn, 1.5f, Fade(UiTheme::PanelBorder(), 0.6f + hoverNextMap * 0.4f));
        Vector2 centerNext = { nextMapBtn.x + nextMapBtn.width/2, nextMapBtn.y + nextMapBtn.height/2 };
        DrawTriangle({centerNext.x + 4, centerNext.y}, {centerNext.x - 4, centerNext.y + 6}, {centerNext.x - 4, centerNext.y - 6}, isHost ? UiTheme::TextPrimary() : UiTheme::TextMuted());

        // Big Map Preview
        DrawRectangleRec(mapPreview, Color{ 18, 20, 28, 200 });
        
        MapData* md = MapRegistry::GetInstance().GetMap(currentMap);
        if (md && md->tmxMap) {
            // Update cache if map changed
            if (cachedPreviewName != currentMap) {
                BeginTextureMode(cachedPreview);
                ClearBackground(Color{ 18, 20, 28, 255 });
                
                float scaleX = mapPreview.width  / (float)md->pixelWidth;
                float scaleY = mapPreview.height / (float)md->pixelHeight;
                float scale  = scaleX < scaleY ? scaleX : scaleY;

                Camera2D miniCam = {};
                miniCam.zoom     = scale;
                miniCam.target   = { 0.0f, 0.0f };
                miniCam.offset   = { (mapPreview.width - md->pixelWidth * scale) / 2.0f, 
                                     (mapPreview.height - md->pixelHeight * scale) / 2.0f };
                miniCam.rotation = 0.0f;

                BeginMode2D(miniCam);
                // Draw full map
                Rectangle mapSpaceVp = { 0, 0, (float)md->pixelWidth, (float)md->pixelHeight };
                DrawTMX(md->tmxMap, &miniCam, &mapSpaceVp, 0, 0, WHITE);
                EndMode2D();
                
                EndTextureMode();
                cachedPreviewName = currentMap;
            }

            // Draw the cached texture (flip Y because OpenGL render textures are inverted)
            Rectangle sourceRec = { 0.0f, 0.0f, (float)cachedPreview.texture.width, -(float)cachedPreview.texture.height };
            DrawTextureRec(cachedPreview.texture, sourceRec, { mapPreview.x, mapPreview.y }, WHITE);
        } else {
            DrawTextEx(menuFont, "No preview",
                       { mapPreview.x + 140, mapPreview.y + 40 }, 12.0f, 1.0f, UiTheme::TextMuted());
        }
        
        DrawRectangleLinesEx(mapPreview, 1.5f, UiTheme::PanelBorder());
    }

    DrawModeSelector();
    DrawChipRows();

    bool localReady = NetworkManager::GetInstance().IsLocalPlayerReady();
    bool canStart = isHost && NetworkManager::GetInstance().AllPlayersReady();

    Ui::DrawMenuButton(readyBtn, menuFont,
                       localReady ? "UNREADY  (R)" : "READY  (R)",
                       hoverReady, !localReady, true);

    if (isHost) {
        Ui::DrawMenuButton(startBtn, menuFont, "START MATCH  (SPACE)",
                           hoverStart, true, canStart);

        DrawTextEx(menuFont, "SHARE", { 780, copyBtn.y - 14 }, 12.0f, 1.0f, UiTheme::TextMuted());
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
                   { 780, readyBtn.y + readyBtn.height + 12 }, 14.0f, 1.0f, UiTheme::TextMuted());
    }

    // Back button like in OnlineMenu (top-left)
    Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    Ui::DrawMenuButton(globalBackBtn, menuFont, "BACK", hoverGlobalBack, false, true);

    EndTextureMode();
}

