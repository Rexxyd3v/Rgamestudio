#include "character_preview.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "../utils/texture_manager.h"
#include <cmath>

std::string CharacterPreview::BodyPath(int skin) {
    if (skin < 1) skin = 1;
    if (skin > 4) skin = 4;
    return std::string(
        "assets/Free 2D Animated Vector Game Character Sprites/"
        "Free 2D Animated Vector Game Character Sprites/"
        "Full body animated characters/Char ") +
        std::to_string(skin) + "/with hands/";
}

std::string CharacterPreview::HeadPath(int skin) {
    if (skin < 1) skin = 1;
    if (skin > 4) skin = 4;
    return "assets/Head_display/char" + std::to_string(skin) + ".png";
}

CharacterPreview::CharacterPreview() {
    for (int i = 0; i < kCount; ++i) {
        std::string base = BodyPath(i + 1);
        idle[i] = new Animation(base + "idle_", 6, 0.12f, true);
        heads[i] = TextureManager::GetTexture(HeadPath(i + 1));
        if (heads[i].id == 0) {
            heads[i] = TextureManager::GetTexture(HeadPath(1));
        }
    }
}

CharacterPreview::~CharacterPreview() {
    for (int i = 0; i < kCount; ++i) {
        delete idle[i];
        idle[i] = nullptr;
    }
}

void CharacterPreview::Update(float deltaTime) {
    pulse += deltaTime;
    for (int i = 0; i < kCount; ++i) {
        if (idle[i]) idle[i]->Update(deltaTime);
    }
}

void CharacterPreview::DrawHead(int skin, Rectangle bounds) const {
    int idx = skin - 1;
    if (idx < 0 || idx >= kCount) idx = 0;
    Texture2D tex = heads[idx];
    if (tex.id == 0) {
        DrawRectangleRec(bounds, Fade(UiTheme::ButtonIdle(), 0.8f));
        return;
    }
    float scale = fminf(bounds.width / (float)tex.width, bounds.height / (float)tex.height);
    float w = tex.width * scale;
    float h = tex.height * scale;
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = {
        bounds.x + (bounds.width - w) * 0.5f,
        bounds.y + (bounds.height - h) * 0.5f,
        w, h
    };
    DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
}

void CharacterPreview::DrawStage(Rectangle stageBounds, int selectedSkin) {
    int idx = selectedSkin - 1;
    if (idx < 0 || idx >= kCount) idx = 0;

    Rectangle stageBox = stageBounds;
    const float topPadding = 28.0f;
    stageBox.y += topPadding;
    stageBox.height -= topPadding;


    DrawRectangleRec(stageBox, Fade(BLACK, 0.30f));
    DrawRectangleLinesEx(stageBox, 1.5f, Fade(UiTheme::AccentGold(), 0.35f));

    if (idle[idx]) {
        // Dynamically scale character based on stage height, capped at original scale
        float baseScale = 0.16f;
        float scale = baseScale * 2.0f * (stageBox.height / 292.0f);
        if (scale > baseScale * 2.0f) scale = baseScale * 2.0f;

        Vector2 pos = {
            stageBox.x + stageBox.width * 0.5f,
            stageBox.y + stageBox.height * 0.12f
        };
        idle[idx]->Draw(pos, 1, scale, 0.0f);
    }
}

int CharacterPreview::DrawSelector(Rectangle area, const Font& font, int selectedSkin, Vector2 mouse) {
    int clicked = 0;
    const float gap = 12.0f;
    float cardW = (area.width - gap * (kCount - 1)) / (float)kCount;
    float cardH = area.height;

    for (int i = 0; i < kCount; ++i) {
        Rectangle card = {
            area.x + i * (cardW + gap),
            area.y,
            cardW,
            cardH
        };
        bool selected = (selectedSkin == i + 1);
        bool hovered = CheckCollisionPointRec(mouse, card);

        Color fill = selected ? Color{ 40, 36, 28, 230 } : Color{ 22, 24, 32, 200 };
        if (hovered && !selected) fill = Color{ 34, 36, 48, 220 };
        DrawRectangleRec(card, fill);

        Color border = selected ? UiTheme::AccentGold() : UiTheme::PanelBorder();
        float borderThick = selected ? 3.0f : (hovered ? 2.0f : 1.5f);
        DrawRectangleLinesEx(card, borderThick, Fade(border, selected ? 1.0f : 0.55f));

        if (selected) {
            float pulseA = 0.35f + 0.25f * sinf(pulse * 3.0f);
            DrawRectangleLinesEx(
                { card.x - 3, card.y - 3, card.width + 6, card.height + 6 },
                1.5f, Fade(UiTheme::AccentGold(), pulseA));
        }

        // Character body preview inside card
        Rectangle bodyArea = {
            card.x + 4,
            card.y + 8,
            card.width - 8,
            card.height - 36
        };
        if (idle[i]) {
            Vector2 pos = {
                bodyArea.x + bodyArea.width * 0.5f,
                bodyArea.y + bodyArea.height * 0.25f // Moved up slightly
            };
            float baseScale = 0.08f;
            float scale = baseScale * 1.4f * fminf(bodyArea.width / 90.0f, bodyArea.height / 110.0f);
            idle[i]->Draw(pos, 1, scale, 0.0f);
        }

        std::string label = "CHAR " + std::to_string(i + 1);
        Vector2 lsz = MeasureTextEx(font, label.c_str(), 12.0f, 1.0f);
        DrawTextEx(font, label.c_str(),
                   { card.x + card.width * 0.5f - lsz.x * 0.5f, card.y + card.height - 22 },
                   12.0f, 1.0f,
                   selected ? UiTheme::AccentGold() : UiTheme::TextMuted());

        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = i + 1;
        }
    }
    return clicked;
}
