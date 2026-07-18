#include "online_menu.h"
#include "../network/network_manager.h"
#include "../constants.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_theme.h"
#include <cmath>
#include <string>

OnlineMenuScreen::OnlineMenuScreen() :
    startGame(false),
    goToLobby(false),
    activeInput(0),
    joinAddress(""),
    username("Player"),
    currentSkin(1),
    isConnecting(false),
    connectError(""),
    background(nullptr),
    preview(nullptr),
    fadeIn(0.0f),
    blurStrength(0.75f),
    hoverHost(0.0f),
    hoverJoin(0.0f),
    hoverGlobalBack(0.0f),
    caretBlink(0.0f) {

    menuFont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 60, nullptr, 0);
    if (menuFont.texture.id == 0) menuFont = GetFontDefault();

    if (NetworkManager::GetInstance().localSkinIndex >= 1 &&
        NetworkManager::GetInstance().localSkinIndex <= 4) {
        currentSkin = NetworkManager::GetInstance().localSkinIndex;
    }
    if (!NetworkManager::GetInstance().localUsername.empty()) {
        username = NetworkManager::GetInstance().localUsername;
    }

    background = new MenuBackground(currentSkin, MenuBackground::BackdropStyle::CINEMATIC_BEACH);
    preview = new CharacterPreview();

    shell = { 70.0f, 70.0f, 1160.0f, 540.0f };
    stageArea = { 110.0f, 130.0f, 520.0f, 320.0f };
    selectorArea = { 110.0f, 470.0f, 520.0f, 100.0f };
    nameField = { 720.0f, 220.0f, 420.0f, 48.0f };
    hostBtn = { 720.0f, 340.0f, 420.0f, 64.0f };
    joinBtn = { 720.0f, 420.0f, 420.0f, 64.0f };
}

OnlineMenuScreen::~OnlineMenuScreen() {
    delete preview;
    delete background;
    if (menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(menuFont);
    }
}

bool OnlineMenuScreen::Update(float deltaTime) {
    fadeIn = Ui::Approach(fadeIn, 1.0f, 2.0f, deltaTime);
    blurStrength = Ui::Approach(blurStrength, 0.45f, 0.4f, deltaTime);
    caretBlink += deltaTime * 2.0f;
    if (caretBlink > 1.0f) caretBlink -= 1.0f;

    if (background) background->Update(deltaTime, fadeIn);
    if (preview) preview->Update(deltaTime);

    if (isConnecting) {
        auto state = NetworkManager::GetInstance().PollJoinRoom();
        if (state == NetworkManager::JoinState::CONNECTED) {
            isConnecting = false;
            goToLobby = true;
            return false;
        } else if (state == NetworkManager::JoinState::FAILED) {
            isConnecting = false;
            connectError = "Connection failed. Check the address and try again.";
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            NetworkManager::GetInstance().Disconnect();
            isConnecting = false;
            connectError = "Connection cancelled.";
        }
        return true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (activeInput != 0) {
            activeInput = 0;
        } else {
            return false;
        }
    }

    Vector2 mouse = Ui::RemapMouseToVirtual();

    // Character select always available (including during join address entry)
    {
        const float gap = 12.0f;
        float cardW = (selectorArea.width - gap * 3) / 4.0f;
        for (int i = 0; i < 4; ++i) {
            Rectangle card = {
                selectorArea.x + i * (cardW + gap),
                selectorArea.y,
                cardW,
                selectorArea.height
            };
            if (CheckCollisionPointRec(mouse, card) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentSkin = i + 1;
                NetworkManager::GetInstance().localSkinIndex = currentSkin;
                connectError = "";
            }
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

            if (IsKeyPressed(KEY_BACKSPACE) && !activeStr->empty()) {
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
                    connectError = "";
                    NetworkManager::GetInstance().localUsername = username;
                    NetworkManager::GetInstance().localSkinIndex = currentSkin;
                    if (NetworkManager::GetInstance().BeginJoinRoom(joinAddress)) {
                        isConnecting = true;
                    } else {
                        connectError = "Invalid address.";
                    }
                    activeInput = 0;
                } else if (activeInput == 1) {
                    activeInput = 0;
                }
            }
        }

        // Still allow BACK while editing
        Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
        bool hBack = CheckCollisionPointRec(mouse, globalBackBtn);
        hoverGlobalBack = Ui::Approach(hoverGlobalBack, hBack ? 1.0f : 0.0f, 8.0f, deltaTime);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hBack) {
            activeInput = 0;
        }
        return true;
    }

    bool hHost = CheckCollisionPointRec(mouse, hostBtn);
    bool hJoin = CheckCollisionPointRec(mouse, joinBtn);
    Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    bool hBack = CheckCollisionPointRec(mouse, globalBackBtn);

    hoverHost = Ui::Approach(hoverHost, hHost ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverJoin = Ui::Approach(hoverJoin, hJoin ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverGlobalBack = Ui::Approach(hoverGlobalBack, hBack ? 1.0f : 0.0f, 8.0f, deltaTime);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        connectError = "";
        if (hBack) {
            return false;
        } else if (CheckCollisionPointRec(mouse, nameField)) {
            activeInput = 1;
        } else if (hHost) {
            NetworkManager::GetInstance().localUsername = username;
            NetworkManager::GetInstance().localSkinIndex = currentSkin;
            if (NetworkManager::GetInstance().HostRoom(DEFAULT_GAME_PORT)) {
                goToLobby = true;
                return false;
            }
            connectError = "Failed to host room.";
        } else if (hJoin) {
            activeInput = 2;
        }
    }

    return true;
}

void OnlineMenuScreen::Draw(RenderTexture2D target) {
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

    Ui::DrawSectionLabel(menuFont, "ONLINE", { shell.x + 36, shell.y + 22 }, 28.0f);
    DrawTextEx(menuFont, "Choose your fighter, then host or join",
               { shell.x + 36, shell.y + 56 }, 14.0f, 1.0f, UiTheme::TextMuted());

    // Connecting overlay
    if (isConnecting) {
        DrawRectangleRec(shell, UiTheme::DimOverlay());
        Ui::DrawCenteredText(menuFont, "CONNECTING", VIRTUAL_HEIGHT * 0.42f, 36.0f, UiTheme::AccentGold());
        float spin = (float)GetTime() * 4.0f;
        Vector2 c = { VIRTUAL_WIDTH * 0.5f, VIRTUAL_HEIGHT * 0.55f };
        for (int i = 0; i < 8; ++i) {
            float a = spin + i * (PI * 0.25f);
            float r = 28.0f;
            DrawCircle(
                (int)(c.x + cosf(a) * r),
                (int)(c.y + sinf(a) * r),
                4.0f, Fade(UiTheme::AccentGold(), 0.3f + (i / 8.0f) * 0.7f));
        }
        Ui::DrawCenteredText(menuFont, "ESC to cancel", VIRTUAL_HEIGHT * 0.62f, 16.0f, UiTheme::TextMuted());
        EndTextureMode();
        return;
    }

    // Left: character stage + selector
    Ui::DrawSectionLabel(menuFont, "CHOOSE CHARACTER", { stageArea.x, stageArea.y - 22 }, 14.0f);
    if (preview) {
        preview->DrawStage(stageArea, currentSkin);
        Vector2 mouse = Ui::RemapMouseToVirtual();
        preview->DrawSelector(selectorArea, menuFont, currentSkin, mouse);
    }

    std::string charLabel = "CHAR " + std::to_string(currentSkin);
    Vector2 charTextSize = MeasureTextEx(menuFont, charLabel.c_str(), 14.0f, 1.0f);
    DrawTextEx(menuFont, charLabel.c_str(),
               { stageArea.x + stageArea.width - charTextSize.x, stageArea.y - 22 },
               14.0f, 1.0f, UiTheme::AccentGold());

    // Right panel content
    if (activeInput == 2) {
        Ui::DrawSectionLabel(menuFont, "JOIN ROOM", { 720, 160 }, 22.0f);
        DrawTextEx(menuFont, "Server address (LAN or playit.gg)",
                   { 720, 195 }, 14.0f, 1.0f, UiTheme::TextMuted());

        Rectangle addrField = { 720, 230, 420, 48 };
        Ui::DrawTextField(addrField, menuFont, joinAddress.c_str(), true, caretBlink);

        DrawTextEx(menuFont, "Examples: 192.168.1.5   or   abc.gl.at.ply.gg:54321",
                   { 720, 295 }, 12.0f, 1.0f, UiTheme::TextMuted());
        DrawTextEx(menuFont, "ENTER connect   -   ESC cancel",
                   { 720, 330 }, 14.0f, 1.0f, UiTheme::TextMuted());
    } else {
        Ui::DrawSectionLabel(menuFont, "YOUR SETUP", { 720, 160 }, 22.0f);

        DrawTextEx(menuFont, "NAME", { 720, 195 }, 12.0f, 1.0f, UiTheme::TextMuted());
        Ui::DrawTextField(nameField, menuFont, username.c_str(), activeInput == 1, caretBlink);

        Ui::DrawMenuButton(hostBtn, menuFont, "CREATE ROOM", hoverHost, true, true);
        Ui::DrawMenuButton(joinBtn, menuFont, "JOIN ROOM", hoverJoin, false, true);

        DrawTextEx(menuFont, "Host: open UDP 7777 (playit.gg tunnel OK)",
                   { 720, 505 }, 12.0f, 1.0f, UiTheme::TextMuted());
        DrawTextEx(menuFont, "Join: paste host:port or LAN IP",
                   { 720, 525 }, 12.0f, 1.0f, UiTheme::TextMuted());
    }

    Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    Ui::DrawMenuButton(globalBackBtn, menuFont, "BACK", hoverGlobalBack, false, true);

    Ui::DrawStatusBanner(menuFont, connectError.c_str(), true);

    EndTextureMode();
}
