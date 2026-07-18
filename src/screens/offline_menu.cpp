#include "offline_menu.h"
#include "../network/network_manager.h"
#include "../constants.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_theme.h"
#include <cmath>
#include <string>

OfflineMenuScreen::OfflineMenuScreen() :
    fadeIn(0.0f),
    blurStrength(0.75f),
    caretBlink(0.0f),
    currentLeftTab(0),
    hoverTabFighter(1.0f),
    hoverTabLoadout(0.0f),
    activeInput(0),
    username("Player"),
    currentSkin(1),
    currentGunSkin(0),
    background(nullptr),
    preview(nullptr),
    skinPreview(nullptr),
    hoverStart(0.0f),
    hoverBack(0.0f),
    startGame(false)
{
    // Load font
    menuFont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 60, nullptr, 0);
    if (menuFont.texture.id == 0) menuFont = GetFontDefault();

    // Initialize from network manager (which holds last used values)
    if (NetworkManager::GetInstance().localSkinIndex >= 1 &&
        NetworkManager::GetInstance().localSkinIndex <= 4) {
        currentSkin = NetworkManager::GetInstance().localSkinIndex;
    }
    if (!NetworkManager::GetInstance().localUsername.empty()) {
        username = NetworkManager::GetInstance().localUsername;
    }
    // Note: Weapon skin is handled per slot in WeaponSkinPreview, but we also store a global for simplicity?
    // Actually, the WeaponSkinPreview manages per-slot skins. We'll initialize it with the current global weapon skin
    // as a starting point, but the preview will allow per-slot selection.
    currentGunSkin = NetworkManager::GetInstance().localWeaponSkin;

    // Create background and previews
    background = new MenuBackground(currentSkin, MenuBackground::BackdropStyle::CINEMATIC_BEACH);
    preview = new CharacterPreview();
    skinPreview = new WeaponSkinPreview();

    // Layout (copied from OnlineMenuScreen, adjusted for no networking buttons)
    shell = { 70.0f, 70.0f, 1160.0f, 540.0f };

    // Left side: character/gun tabs and preview
    stageArea = { 110.0f, 195.0f, 520.0f, 255.0f }; // character stage
    selectorArea = { 110.0f, 465.0f, 520.0f, 100.0f }; // character selector
    // Offset gunPreviewArea vertically by 28.0f to match CharacterPreview's inner stageBox padding
    gunPreviewArea = { stageArea.x, stageArea.y + 28.0f, stageArea.width, stageArea.height - 28.0f };
    gunSelectorArea = selectorArea;

    // Right side: name field and start button
    float rightX = 720.0f;
    float rightWidth = 420.0f;
    nameField = { rightX, 230.0f, rightWidth, 48.0f };
    startBtn = { rightX, 350.0f, rightWidth, 64.0f }; // START button
}

OfflineMenuScreen::~OfflineMenuScreen() {
    delete preview;
    delete background;
    delete skinPreview;
    if (menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(menuFont);
    }
}

bool OfflineMenuScreen::Update(float deltaTime) {
    // Update animations
    fadeIn = Ui::Approach(fadeIn, 1.0f, 2.0f, deltaTime);
    blurStrength = Ui::Approach(blurStrength, 0.45f, 0.4f, deltaTime);
    caretBlink += deltaTime * 2.0f;
    if (caretBlink > 1.0f) caretBlink -= 1.0f;

    // Update background and previews
    if (background) background->Update(deltaTime, fadeIn);
    if (preview) preview->Update(deltaTime);
    if (skinPreview) skinPreview->Update(deltaTime);

    // Handle ESC key
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (activeInput != 0) {
            activeInput = 0;
        } else {
            // Go back to main menu
            return false;
        }
    }

    Vector2 mouse = Ui::RemapMouseToVirtual();

    // Tabs on left side (Sleek text-based tabs)
    float fontSize = 16.0f;
    Vector2 szFighter = MeasureTextEx(menuFont, "FIGHTER", fontSize, 1.0f);
    Vector2 szLoadout = MeasureTextEx(menuFont, "LOADOUT", fontSize, 1.0f);

    Rectangle tabFighter = { 110.0f, 145.0f, szFighter.x + 20.0f, 30.0f };
    Rectangle tabLoadout = { 110.0f + szFighter.x + 40.0f, 145.0f, szLoadout.x + 20.0f, 30.0f };

    bool hTabFighter = CheckCollisionPointRec(mouse, tabFighter);
    bool hTabLoadout = CheckCollisionPointRec(mouse, tabLoadout);

    hoverTabFighter = Ui::Approach(hoverTabFighter, (hTabFighter || currentLeftTab == 0) ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverTabLoadout = Ui::Approach(hoverTabLoadout, (hTabLoadout || currentLeftTab == 1) ? 1.0f : 0.0f, 8.0f, deltaTime);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hTabFighter) {
            currentLeftTab = 0;
        }
        if (hTabLoadout) {
            currentLeftTab = 1;
        }
    }

    if (currentLeftTab == 0) {
        // Character select
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
                // Update weapon skin preview to show weapons for the selected character
                if (skinPreview) {
                    skinPreview->SetSkin(currentSkin);
                }
                connectError = "";
            }
        }
    } else {
        // Gun skin selection is handled in Draw method via WeaponSkinPreview::DrawSelector
        // We'll let the preview handle clicks internally for per-slot selection
    }

    // Handle name field input
    if (activeInput != 0) {
        if (activeInput == 1) { // name field
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (username.length() < 64)) {
                    username += (char)key;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && !username.empty()) {
                username.pop_back();
            }

            if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
                const char* clipboardText = GetClipboardText();
                if (clipboardText) {
                    std::string clip(clipboardText);
                    for (char c : clip) {
                        if ((c >= 32) && (c <= 125) && (username.length() < 64)) {
                            username += c;
                        }
                    }
                }
            }

            if (IsKeyPressed(KEY_ENTER)) {
                activeInput = 0;
            }
        }

        // Allow clicking outside to deselect
        Rectangle globalBackBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
        bool hBack = CheckCollisionPointRec(mouse, globalBackBtn);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hBack) {
            activeInput = 0;
        }
        return true;
    }

    // Button hovers
    bool hStart = CheckCollisionPointRec(mouse, startBtn);
    bool hBack = CheckCollisionPointRec(mouse, { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f });

    hoverStart = Ui::Approach(hoverStart, hStart ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverBack = Ui::Approach(hoverBack, hBack ? 1.0f : 0.0f, 8.0f, deltaTime);

    // Handle button clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        connectError = "";
        if (hBack) {
            // Back button clicked - return to main menu
            return false;
        } else if (CheckCollisionPointRec(mouse, nameField)) {
            // Name field clicked - activate text input
            activeInput = 1;
        } else if (hStart) {
            // Start button clicked - initialize game and prepare to transition
            NetworkManager::GetInstance().localUsername = username;
            NetworkManager::GetInstance().localSkinIndex = currentSkin;

            // Get the currently selected weapon skin for the active slot (SMG is slot 0)
            int slot = skinPreview->GetSelectedWeaponSlot();
            int skinId = skinPreview->GetWeaponSkin(slot);
            NetworkManager::GetInstance().localWeaponSkin = skinId;

            startGame = true;
            return true; // Remain on this screen; main.cpp will detect startGame and transition
        }
    }

    return true;
}

void OfflineMenuScreen::Draw(RenderTexture2D target) {
    // Background
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

    Ui::DrawSectionLabel(menuFont, "OFFLINE", { shell.x + 36, shell.y + 22 }, 28.0f);
    DrawTextEx(menuFont, "Choose your fighter and loadout",
               { shell.x + 36, shell.y + 56 }, 14.0f, 1.0f, UiTheme::TextMuted());

    // Left Tabs Drawing (Sleek, tab-bar style with thin underline) - copied from OnlineMenuScreen
    float tabFontSize = 16.0f;
    Vector2 szFighter = MeasureTextEx(menuFont, "FIGHTER", tabFontSize, 1.0f);
    Vector2 szLoadout = MeasureTextEx(menuFont, "LOADOUT", tabFontSize, 1.0f);

    Vector2 posFighter = { 110.0f, 150.0f };
    Vector2 posLoadout = { 110.0f + szFighter.x + 40.0f, 150.0f };
    float lineY = 175.0f;

    // Draw bottom border line for the whole tab bar
    DrawLineEx({ 110.0f, lineY }, { 630.0f, lineY }, 1.0f, Fade(UiTheme::PanelBorder(), 0.35f));

    // Interpolate colors based on active and hover states
    Color colFighter = UiTheme::TextMuted();
    if (currentLeftTab == 0) {
        colFighter = UiTheme::AccentGold();
    } else {
        colFighter.r = (unsigned char)(UiTheme::TextMuted().r + (UiTheme::TextPrimary().r - UiTheme::TextMuted().r) * hoverTabFighter);
        colFighter.g = (unsigned char)(UiTheme::TextMuted().g + (UiTheme::TextPrimary().g - UiTheme::TextMuted().g) * hoverTabFighter);
        colFighter.b = (unsigned char)(UiTheme::TextMuted().b + (UiTheme::TextPrimary().b - UiTheme::TextMuted().b) * hoverTabFighter);
    }

    Color colLoadout = UiTheme::TextMuted();
    if (currentLeftTab == 1) {
        colLoadout = UiTheme::AccentGold();
    } else {
        colLoadout.r = (unsigned char)(UiTheme::TextMuted().r + (UiTheme::TextPrimary().r - UiTheme::TextMuted().r) * hoverTabLoadout);
        colLoadout.g = (unsigned char)(UiTheme::TextMuted().g + (UiTheme::TextPrimary().g - UiTheme::TextMuted().g) * hoverTabLoadout);
        colLoadout.b = (unsigned char)(UiTheme::TextMuted().b + (UiTheme::TextPrimary().b - UiTheme::TextMuted().b) * hoverTabLoadout);
    }

    // Draw Tab Labels
    DrawTextEx(menuFont, "FIGHTER", posFighter, tabFontSize, 1.0f, colFighter);
    DrawTextEx(menuFont, "LOADOUT", posLoadout, tabFontSize, 1.0f, colLoadout);

    // Draw Tab Underlines
    if (currentLeftTab == 0) {
        DrawRectangleRec({ posFighter.x - 2.0f, lineY - 2.0f, szFighter.x + 4.0f, 3.0f }, UiTheme::AccentGold());
    } else if (hoverTabFighter > 0.01f) {
        DrawRectangleRec({ posFighter.x - 2.0f, lineY - 1.0f, szFighter.x + 4.0f, 2.0f }, Fade(UiTheme::TextPrimary(), hoverTabFighter * 0.4f));
    }

    if (currentLeftTab == 1) {
        DrawRectangleRec({ posLoadout.x - 2.0f, lineY - 2.0f, szLoadout.x + 4.0f, 3.0f }, UiTheme::AccentGold());
    } else if (hoverTabLoadout > 0.01f) {
        DrawRectangleRec({ posLoadout.x - 2.0f, lineY - 1.0f, szLoadout.x + 4.0f, 2.0f }, Fade(UiTheme::TextPrimary(), hoverTabLoadout * 0.4f));
    }

    // Content based on active tab
    if (currentLeftTab == 0) {
        // Left: character stage + selector
        if (preview) {
            preview->DrawStage(stageArea, currentSkin);
            Vector2 mouse = Ui::RemapMouseToVirtual();
            preview->DrawSelector(selectorArea, menuFont, currentSkin, mouse);
        }

        std::string charLabel = "CHAR " + std::to_string(currentSkin);
        Vector2 charTextSize = MeasureTextEx(menuFont, charLabel.c_str(), 14.0f, 1.0f);
        DrawTextEx(menuFont, charLabel.c_str(),
                   { stageArea.x + stageArea.width - charTextSize.x, stageArea.y + 6.0f },
                   14.0f, 1.0f, UiTheme::AccentGold());
    } else {
        // Left: gun skin preview and selector
        if (skinPreview) {
            skinPreview->DrawPreview(gunPreviewArea, 0, menuFont);
            Vector2 mouse = Ui::RemapMouseToVirtual();

            // Get active weapon slot and current skin for that slot
            int slot = skinPreview->GetSelectedWeaponSlot();
            int currentWeaponSkin = skinPreview->GetWeaponSkin(slot);

            int clicked = skinPreview->DrawSelector(gunSelectorArea, menuFont, currentWeaponSkin, mouse);
            if (clicked != -1) {
                // Save selected skin for this weapon slot to NetworkManager (for local player info sync)
                NetworkManager::GetInstance().localWeaponSkin = currentWeaponSkin;
                connectError = "";
            }

            // Draw Weapon Skin Label showing active weapon + active skin
            std::string weaponName = "SMG";
            if (slot == 1) weaponName = "SHOTGUN";
            else if (slot == 2) weaponName = "PISTOL";

            std::string skinLabel = weaponName + " SKIN: ";
            if (currentWeaponSkin == 0) {
                skinLabel += "DEFAULT";
            } else if (slot == 0) {
                if (currentWeaponSkin == 1) skinLabel += "NEON CAMO";
                else if (currentWeaponSkin == 2) skinLabel += "MAGMA";
                else if (currentWeaponSkin == 3) skinLabel += "CARBON FIBER";
            } else if (slot == 1) {
                if (currentWeaponSkin == 1) skinLabel += "CLASSIC WOOD";
                else if (currentWeaponSkin == 2) skinLabel += "GOLDEN OAK";
            } else if (slot == 2) {
                if (currentWeaponSkin == 1) skinLabel += "SILVER ALLOY";
                else if (currentWeaponSkin == 2) skinLabel += "NEON CYBER";
            }

            Vector2 skinTextSize = MeasureTextEx(menuFont, skinLabel.c_str(), 14.0f, 1.0f);
            DrawTextEx(menuFont, skinLabel.c_str(),
                       { stageArea.x + stageArea.width - skinTextSize.x, stageArea.y + 6.0f },
                       14.0f, 1.0f, UiTheme::AccentGold());
        }
    }

    // Right panel content (similar to OnlineMenuScreen's "YOUR SETUP" mode)
    Ui::DrawSectionLabel(menuFont, "YOUR SETUP", { 720, 160 }, 22.0f);

    DrawTextEx(menuFont, "NAME", { 720, 195 }, 12.0f, 1.0f, UiTheme::TextMuted());
    Ui::DrawTextField(nameField, menuFont, username.c_str(), activeInput == 1, caretBlink);

    Ui::DrawMenuButton(startBtn, menuFont, "START", hoverStart, true, true);
    Rectangle backBtn = { 28.0f, VIRTUAL_HEIGHT - 60.0f, 150.0f, 42.0f };
    Ui::DrawMenuButton(backBtn, menuFont, "BACK", hoverBack, false, true);

    // Optional: add some helper text
    DrawTextEx(menuFont, "Enter your name and press START to play offline",
               { 720, 280 }, 12.0f, 1.0f, UiTheme::TextMuted());

    Ui::DrawStatusBanner(menuFont, connectError.c_str(), true);

    EndTextureMode();
}