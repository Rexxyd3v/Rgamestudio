#include "main_menu.h"
#include "../network/network_manager.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_theme.h"

MainMenuScreen::MainMenuScreen() {
    menuFont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 80, nullptr, 0);
    if (menuFont.texture.id == 0) {
        menuFont = GetFontDefault();
    }

    selectedMode = GameMode::NONE;
    hoverOffline = 0.0f;
    hoverOnline = 0.0f;
    uiRise = 0.0f;

    cardOffline = { 720.0f, 250.0f, 480.0f, 110.0f };
    cardOnline  = { 720.0f, 390.0f, 480.0f, 110.0f };

    int skinIdx = NetworkManager::GetInstance().localSkinIndex;
    if (skinIdx < 1 || skinIdx > 4) skinIdx = 1;
    background = new MenuBackground(skinIdx, MenuBackground::BackdropStyle::CINEMATIC_BEACH);

    // Start fully black — background pre-warms invisibly for ~0.6s so bots are
    // already mid-battle when the scene fades in. Prevents the jarring snap.
    fadeInTimer = -0.6f;
    blurStrength = 1.0f;   // start fully blurred, ease to target
    uiRise = 0.0f;

    mainMenuMusic = LoadMusicStream("assets/sounds/MAINSOUNDTRACK.wav");
    SetMusicVolume(mainMenuMusic, 0.0f);
    PlayMusicStream(mainMenuMusic);

    choiceSound = LoadSound("assets/sounds/choice.wav");
}

MainMenuScreen::~MainMenuScreen() {
    if (background) {
        delete background;
        background = nullptr;
    }

    if (menuFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(menuFont);
    }

    UnloadMusicStream(mainMenuMusic);
}

bool MainMenuScreen::Update(float deltaTime) {
    UpdateMusicStream(mainMenuMusic);

    // Advance timer (starts at -0.6, crosses 0, then climbs to 1.0)
    fadeInTimer += deltaTime / 1.6f;
    if (fadeInTimer > 1.0f) fadeInTimer = 1.0f;

    // Music and visuals only start once we cross zero (pre-warm phase is silent)
    float visibleAlpha = (fadeInTimer < 0.0f) ? 0.0f : fadeInTimer;
    SetMusicVolume(mainMenuMusic, visibleAlpha);

    blurStrength = Ui::Approach(blurStrength, 0.42f, 0.35f, deltaTime);
    uiRise = Ui::Approach(uiRise, 1.0f, 2.2f, deltaTime);

    if (background) {
        background->Update(deltaTime, fadeInTimer);
    }

    Vector2 mouse = Ui::RemapMouseToVirtual();
    bool hOff = CheckCollisionPointRec(mouse, cardOffline);
    bool hOn  = CheckCollisionPointRec(mouse, cardOnline);
    hoverOffline = Ui::Approach(hoverOffline, hOff ? 1.0f : 0.0f, 8.0f, deltaTime);
    hoverOnline  = Ui::Approach(hoverOnline,  hOn  ? 1.0f : 0.0f, 8.0f, deltaTime);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (hOff) {
            selectedMode = GameMode::OFFLINE;
            PlaySound(choiceSound);
            return false;
        }
        if (hOn) {
            selectedMode = GameMode::ONLINE;
            PlaySound(choiceSound);
            return false;
        }
    }

    return true;
}

void MainMenuScreen::Draw(RenderTexture2D target) {
    // Clamp to [0,1] for all visual operations
    float visibleAlpha = (fadeInTimer < 0.0f) ? 0.0f : fadeInTimer;

    if (background) {
        background->Draw(target, visibleAlpha, blurStrength);
    } else {
        BeginTextureMode(target);
        ClearBackground(BLACK);
        EndTextureMode();
    }

    BeginTextureMode(target);

    // Full black overlay during pre-warm, then fades out as visibleAlpha climbs
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                  Fade(BLACK, 1.0f - visibleAlpha));

    Ui::DrawVignette(0.7f);

    float risePx    = (1.0f - uiRise) * 24.0f;
    float brandAlpha = visibleAlpha * uiRise;

    // Left brand column
    float titleFontSize = 92.0f;
    float titleSpacing = 4.0f;
    const char* titleText = "VANTA";
    Vector2 titleSize = MeasureTextEx(menuFont, titleText, titleFontSize, titleSpacing);

    DrawTextEx(menuFont, titleText,
               { 72.0f, 170.0f + risePx },
               titleFontSize, titleSpacing, Fade(UiTheme::TextPrimary(), brandAlpha));

    // Underline now matches the measured width of the title text (plus a small margin)
    DrawRectangle(74, (int)(170.0f + titleSize.y + 6.0f + risePx),
                  (int)titleSize.x, 4,
                  Fade(UiTheme::AccentGold(), brandAlpha));

    DrawTextEx(menuFont, "No mercy. No cover. Only the fast survive.",
               { 74.0f, 170.0f + titleSize.y + 26.0f + risePx },
               18.0f, 1.0f, Fade(UiTheme::TextMuted(), brandAlpha));

    DrawTextEx(menuFont, "MAIN MENU",
               { 74.0f, 170.0f + titleSize.y + 66.0f + risePx },
               16.0f, 2.0f, Fade(UiTheme::AccentGold(), brandAlpha * 0.85f));

    // Right mode cards
    Rectangle off = cardOffline;
    Rectangle on  = cardOnline;
    off.y += risePx;
    on.y  += risePx;

    Ui::DrawModeCard(off, menuFont, "OFFLINE", "Solo arena  -  fight the bots", hoverOffline);
    Ui::DrawModeCard(on,  menuFont, "ONLINE",  "Host or join  -  play with friends", hoverOnline);

    EndTextureMode();
}
