#include "weapon_skin_preview.h"
#include "../utils/texture_manager.h"
#include "ui_theme.h"
#include <math.h>
#include <filesystem>

// raylib Color constants compatibility
#ifndef DARKRED
#define DARKRED (Color){139, 0, 0, 255}
#endif


WeaponSkinPreview::WeaponSkinPreview(int skinId)
    : skinId(skinId), rotation(0.0f), selectedWeaponSlot(0), hoverPrev(0.0f), hoverNext(0.0f), lastArea{0} {
    selectedSkins[0] = 0;
    selectedSkins[1] = 0;
    selectedSkins[2] = 0;

    // Load weapon textures for the initial skin
    std::string skinPrefix = WeaponSkinPath(static_cast<WeaponSkinId>(skinId));
    if (skinPrefix.empty()) {
        // Fallback to default skin if invalid
        skinPrefix = "assets/WeaponSkins/Default/";
    }
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR1.png"));
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR2.png"));
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR3.png"));
}

WeaponSkinPreview::~WeaponSkinPreview() {
    // Textures are managed by TextureManager, so we don't unload them here.
    // But we should clear the vector to avoid dangling pointers if we ever change the logic.
    weaponTextures.clear();
}

void WeaponSkinPreview::SetSkin(int skinId) {
    if (this->skinId == skinId) return;

    this->skinId = skinId;

    // Unload current textures
    weaponTextures.clear();

    // Load new textures
    std::string skinPrefix = WeaponSkinPath(static_cast<WeaponSkinId>(skinId));
    if (skinPrefix.empty()) {
        // Fallback to default skin if invalid
        skinPrefix = "assets/WeaponSkins/Default/";
    }
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR1.png"));
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR2.png"));
    weaponTextures.push_back(TextureManager::GetTexture(skinPrefix + "weaponR3.png"));
}

void WeaponSkinPreview::Update(float deltaTime) {
    // Slowly rotate the weapons for a nice display
    rotation += deltaTime * 0.5f; // radians per second
    if (rotation > 2 * PI) rotation -= 2 * PI;

    if (lastArea.width > 0.0f) {
        Vector2 mouse = Ui::RemapMouseToVirtual();
        // Place buttons on the left and right inside the panel
        Rectangle btnPrev = { lastArea.x + 16.0f, lastArea.y + lastArea.height * 0.5f - 20.0f, 40.0f, 40.0f };
        Rectangle btnNext = { lastArea.x + lastArea.width - 56.0f, lastArea.y + lastArea.height * 0.5f - 20.0f, 40.0f, 40.0f };

        bool hPrev = CheckCollisionPointRec(mouse, btnPrev);
        bool hNext = CheckCollisionPointRec(mouse, btnNext);

        hoverPrev = Ui::Approach(hoverPrev, hPrev ? 1.0f : 0.0f, 8.0f, deltaTime);
        hoverNext = Ui::Approach(hoverNext, hNext ? 1.0f : 0.0f, 8.0f, deltaTime);
    }
}

void WeaponSkinPreview::DrawPreview(Rectangle area, int skinId, Font font) {
    (void)skinId;
    (void)font; // unused since we draw custom shapes now

    // This preview draws the currently selected weapon-slot skins
    // stored in selectedSkins[]. Do not ignore input skinId here.

    lastArea = area; // Cache area for Update method to check hits


    // Draw background panel
    DrawRectangleRec(area, Fade(BLACK, 0.30f));
    DrawRectangleLinesEx(area, 1.5f, Fade(UiTheme::AccentGold(), 0.35f));




    const float cropX = 350.0f;
    const float cropY = 1150.0f;
    const float cropW = 1400.0f;
    const float cropH = 650.0f;

    // Draw the single active weapon centered in the stage panel
    if (selectedWeaponSlot >= 0 && selectedWeaponSlot < 3) {
        int activeSkin = selectedSkins[selectedWeaponSlot];
        std::string skinPrefix = GetWeaponSlotSkinPath(selectedWeaponSlot, activeSkin);
        std::string texturePath = skinPrefix + "weaponR" + std::to_string(selectedWeaponSlot + 1) + ".png";
        Texture2D tex = TextureManager::GetTexture(texturePath);
        if (tex.id != 0) {
            float weaponScale = 0.26f; // Nice large centered weapon scale (doesn't cut off)
            float w = cropW * weaponScale;
            float h = cropH * weaponScale;

            float x = area.x + (area.width - w) / 2.0f;
            float y = area.y + (area.height - h) / 2.0f;

            Rectangle src = { cropX, cropY, cropW, cropH };
            Rectangle dst = { x, y, w, h };

            // Rotate/float gently
            float offset = sinf(rotation) * 4.0f;
            Vector2 origin = { w / 2.0f, h / 2.0f };
            dst.x += w / 2.0f;
            dst.y += h / 2.0f;

            DrawTexturePro(tex, src, dst, origin, offset, GetWeaponSlotSkinTint(selectedWeaponSlot, activeSkin));


        }
    }

    // Interactive cycle buttons
    Vector2 mouse = Ui::RemapMouseToVirtual();
    Rectangle btnPrev = { area.x + 16.0f, area.y + area.height * 0.5f - 20.0f, 40.0f, 40.0f };
    Rectangle btnNext = { area.x + area.width - 56.0f, area.y + area.height * 0.5f - 20.0f, 40.0f, 40.0f };

    if (CheckCollisionPointRec(mouse, btnPrev) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedWeaponSlot = (selectedWeaponSlot - 1 + 3) % 3;
    }
    if (CheckCollisionPointRec(mouse, btnNext) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedWeaponSlot = (selectedWeaponSlot + 1) % 3;
    }

    // Draw Left Button Box & Triangle Icon
    DrawRectangleRec(btnPrev, Fade(UiTheme::ButtonIdle(), 0.5f + hoverPrev * 0.5f));
    DrawRectangleLinesEx(btnPrev, 1.5f, Fade(hoverPrev > 0.01f ? UiTheme::AccentGold() : UiTheme::PanelBorder(), 0.5f + hoverPrev * 0.5f));
    Color arrowPrevColor = hoverPrev > 0.01f ? UiTheme::AccentGold() : UiTheme::TextPrimary();


    DrawTriangle(
        { btnPrev.x + 14.0f, btnPrev.y + 20.0f },
        { btnPrev.x + 26.0f, btnPrev.y + 29.0f },
        { btnPrev.x + 26.0f, btnPrev.y + 11.0f },
        arrowPrevColor
    );

    // Draw Right Button Box & Triangle Icon
    DrawRectangleRec(btnNext, Fade(UiTheme::ButtonIdle(), 0.5f + hoverNext * 0.5f));
    DrawRectangleLinesEx(btnNext, 1.5f, Fade(hoverNext > 0.01f ? UiTheme::AccentGold() : UiTheme::PanelBorder(), 0.5f + hoverNext * 0.5f));
    Color arrowNextColor = hoverNext > 0.01f ? UiTheme::AccentGold() : UiTheme::TextPrimary();


    DrawTriangle(
        { btnNext.x + 26.0f, btnNext.y + 20.0f },
        { btnNext.x + 14.0f, btnNext.y + 11.0f },
        { btnNext.x + 14.0f, btnNext.y + 29.0f },
        arrowNextColor
    );
}

int WeaponSkinPreview::DrawSelector(Rectangle area, Font font, int& selectedSkin, Vector2 mouse) {
    // Number of skins for the selected weapon slot
    int skinsCount = 3;
    if (selectedWeaponSlot == 0) skinsCount = 4; // SMG template has 4 options

    const float gap = 12.0f;
    float cardW = (area.width - gap * (skinsCount - 1)) / skinsCount;
    int clickedSkin = -1;

    for (int i = 0; i < skinsCount; ++i) {
        bool locked = false;
        if (i > 0) {
            std::string weaponFolderName = "SMG";
            if (selectedWeaponSlot == 1) weaponFolderName = "Shotgun";
            else if (selectedWeaponSlot == 2) weaponFolderName = "Pistol";
            std::string folder = "assets/WeaponSkins/Skin" + std::to_string(i) + "/" + weaponFolderName + "/";
            locked = !std::filesystem::exists(folder);
        }

        Rectangle card = {
            area.x + i * (cardW + gap),
            area.y,
            cardW,
            area.height
        };

        bool isCurrentSelected = (selectedSkins[selectedWeaponSlot] == i);

        // Card background (theme-consistent)
        Color bg = UiTheme::PanelBg();
        bg.a = (unsigned char)(bg.a * 0.85f);

        bool hovered = !locked && CheckCollisionPointRec(mouse, card);

        if (locked) {
            bg = Fade(UiTheme::PanelBg(), 0.55f);
        } else if (isCurrentSelected) {
            bg = Fade(Color{ 40, 36, 28, 255 }, 0.90f);
        } else if (hovered) {
            bg = Fade(Color{ 34, 36, 48, 255 }, 0.88f);
        }

        DrawRectangleRec(card, bg);

        Color border = isCurrentSelected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
        float borderThick = isCurrentSelected ? 2.5f : 1.0f;
        DrawRectangleLinesEx(card, borderThick, Fade(border, isCurrentSelected ? 1.0f : 0.45f));


        // Get label names for skins
        const char* label = "Default";
        if (selectedWeaponSlot == 0) {
            if (i == 1) label = "Neon Camo";
            else if (i == 2) label = "Magma";
            else if (i == 3) label = "Carbon Fiber";
        } else if (selectedWeaponSlot == 1) {
            if (i == 1) label = "Classic Wood";
            else if (i == 2) label = "Golden Oak";
        } else if (selectedWeaponSlot == 2) {
            if (i == 1) label = "Silver Alloy";
            else if (i == 2) label = "Neon Cyber";
        }

        if (!locked) {
            // Draw active weapon icon in the card with wide crop and padding
            std::string skinPrefix = GetWeaponSlotSkinPath(selectedWeaponSlot, i);
            std::string texturePath = skinPrefix + "weaponR" + std::to_string(selectedWeaponSlot + 1) + ".png";
            Texture2D tex = TextureManager::GetTexture(texturePath);
            if (tex.id != 0) {
                const float cropX = 350.0f;
                const float cropY = 1150.0f;
                const float cropW = 1400.0f;
                const float cropH = 650.0f;

                // Scale down slightly more to leave space (width fits within cardW - 30px)
                float scale = (cardW - 30.0f) / cropW; 
                float w = cropW * scale;
                float h = cropH * scale;
                float x = card.x + (card.width - w) / 2.0f;
                float y = card.y + (card.height - h) / 2.0f - 10.0f;

                Rectangle src = { cropX, cropY, cropW, cropH };
                Rectangle dst = { x, y, w, h };
                DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, GetWeaponSlotSkinTint(selectedWeaponSlot, i));
            }
        } else {
            // Draw locked icon style
            DrawRectangleRec(card, Fade(RED, 0.08f));
            Vector2 lockSize = MeasureTextEx(font, "LOCKED", 10.0f, 1.0f);
            DrawTextEx(font, "LOCKED",
                       { card.x + card.width * 0.5f - lockSize.x * 0.5f, card.y + card.height * 0.5f - 15.0f },
                       10.0f, 1.0f, Fade(RED, 0.6f));
        }

        // Draw label name at the bottom
        Vector2 lsz = MeasureTextEx(font, label, 10.0f, 1.0f);
        DrawTextEx(font, label,
                   { card.x + card.width * 0.5f - lsz.x * 0.5f, card.y + card.height - 20.0f },
                   10.0f, 1.0f, isCurrentSelected ? UiTheme::AccentGold() : UiTheme::TextMuted());

        // Click selection
        if (CheckCollisionPointRec(mouse, card) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (!locked) {
                clickedSkin = i;
                selectedSkins[selectedWeaponSlot] = i;
            }
        }
    }

    if (clickedSkin != -1) selectedSkin = clickedSkin;
    return clickedSkin;
}
