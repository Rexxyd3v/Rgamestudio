#include "ui_widgets.h"
#include "ui_theme.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

namespace Ui {

void DrawVignette(float strength) {
    if (strength <= 0.0f) return;
    const int rings = 8;
    for (int i = 0; i < rings; ++i) {
        float t = (float)(i + 1) / (float)rings;
        unsigned char a = (unsigned char)(strength * 28.0f * t);
        int inset = i * 18;
        DrawRectangleLinesEx(
            { (float)inset, (float)inset,
              (float)(VIRTUAL_WIDTH - inset * 2),
              (float)(VIRTUAL_HEIGHT - inset * 2) },
            18.0f, { 0, 0, 0, a });
    }
    DrawRectangle(0, 0, VIRTUAL_WIDTH, 90, { 0, 0, 0, (unsigned char)(40 * strength) });
    DrawRectangle(0, VIRTUAL_HEIGHT - 80, VIRTUAL_WIDTH, 80, { 0, 0, 0, (unsigned char)(55 * strength) });
}

void DrawGlassPanel(Rectangle bounds, float alpha) {
    Color bg = UiTheme::PanelBg();
    bg.a = (unsigned char)(bg.a * alpha);
    Color border = UiTheme::PanelBorder();
    border.a = (unsigned char)(border.a * alpha);
    Color inner = UiTheme::PanelInner();
    inner.a = (unsigned char)(inner.a * alpha);

    DrawRectangleRec(bounds, bg);
    DrawRectangleLinesEx(bounds, 1.5f, border);
    DrawRectangle(
        (int)(bounds.x + 1), (int)(bounds.y + 1),
        (int)(bounds.width - 2), 2, inner);
}

bool DrawMenuButton(Rectangle bounds, const Font& font, const char* label,
                    float hoverT, bool primary, bool enabled) {
    Vector2 mouse = RemapMouseToVirtual();
    bool hovered = enabled && CheckCollisionPointRec(mouse, bounds);

    float scale = 1.0f + hoverT * 0.03f;
    float cx = bounds.x + bounds.width * 0.5f;
    float cy = bounds.y + bounds.height * 0.5f;
    Rectangle draw = {
        cx - bounds.width * scale * 0.5f,
        cy - bounds.height * scale * 0.5f,
        bounds.width * scale,
        bounds.height * scale
    };

    Color fill = enabled
        ? Color{
            (unsigned char)(UiTheme::ButtonIdle().r + (UiTheme::ButtonHover().r - UiTheme::ButtonIdle().r) * hoverT),
            (unsigned char)(UiTheme::ButtonIdle().g + (UiTheme::ButtonHover().g - UiTheme::ButtonIdle().g) * hoverT),
            (unsigned char)(UiTheme::ButtonIdle().b + (UiTheme::ButtonHover().b - UiTheme::ButtonIdle().b) * hoverT),
            (unsigned char)(UiTheme::ButtonIdle().a + (UiTheme::ButtonHover().a - UiTheme::ButtonIdle().a) * hoverT)
          }
        : Color{ 40, 40, 48, 140 };

    DrawRectangleRec(draw, fill);

    Color border = primary ? UiTheme::AccentGold() : UiTheme::PanelBorder();
    if (!enabled) border = Fade(border, 0.35f);
    else border = Fade(border, 0.45f + hoverT * 0.55f);
    DrawRectangleLinesEx(draw, 2.0f, border);

    if (enabled && (primary || hoverT > 0.01f)) {
        float railW = 4.0f + hoverT * 2.0f;
        DrawRectangle((int)draw.x, (int)draw.y, (int)railW, (int)draw.height,
                      Fade(UiTheme::AccentGold(), 0.35f + hoverT * 0.65f));
    }

    float fontSize = UiTheme::FontButton;
    Vector2 sz = MeasureTextEx(font, label, fontSize, 1.0f);
    
    // Scale down text if it's too wide for the button
    float maxTextWidth = draw.width - 32.0f; // 16px padding on each side
    if (sz.x > maxTextWidth) {
        fontSize *= maxTextWidth / sz.x;
        sz = MeasureTextEx(font, label, fontSize, 1.0f);
    }

    Color textCol = enabled ? UiTheme::TextPrimary() : Fade(UiTheme::TextMuted(), 0.5f);
    DrawTextEx(font, label,
               { draw.x + draw.width * 0.5f - sz.x * 0.5f,
                 draw.y + draw.height * 0.5f - sz.y * 0.5f },
               fontSize, 1.0f, textCol);

    return hovered;
}

bool DrawModeCard(Rectangle bounds, const Font& font,
                  const char* title, const char* subtitle, float hoverT) {
    Vector2 mouse = RemapMouseToVirtual();
    bool hovered = CheckCollisionPointRec(mouse, bounds);

    float lift = hoverT * 6.0f;
    Rectangle draw = { bounds.x, bounds.y - lift, bounds.width, bounds.height };

    DrawGlassPanel(draw, 0.92f + hoverT * 0.08f);

    Color accent = Fade(UiTheme::AccentGold(), 0.25f + hoverT * 0.75f);
    DrawRectangle((int)draw.x, (int)draw.y, 5, (int)draw.height, accent);
    DrawRectangleLinesEx(draw, 2.0f, Fade(UiTheme::AccentGold(), 0.2f + hoverT * 0.6f));

    float titleSize = 32.0f;
    Vector2 tsz = MeasureTextEx(font, title, titleSize, 1.0f);
    DrawTextEx(font, title,
               { draw.x + 28.0f, draw.y + 22.0f },
               titleSize, 1.0f, UiTheme::TextPrimary());

    float subSize = 16.0f;
    DrawTextEx(font, subtitle,
               { draw.x + 28.0f, draw.y + 22.0f + tsz.y + 6.0f },
               subSize, 1.0f, UiTheme::TextMuted());

    return hovered;
}

void DrawTextField(Rectangle bounds, const Font& font, const char* text,
                   bool focused, float caretBlink) {
    Color fill = focused ? Color{ 245, 240, 230, 245 } : Color{ 220, 215, 205, 220 };
    DrawRectangleRec(bounds, fill);
    DrawRectangleLinesEx(bounds, focused ? 2.5f : 1.5f,
                         focused ? UiTheme::AccentGold() : UiTheme::PanelBorder());

    const float pad = 12.0f;
    float fontSize = 20.0f;
    DrawTextEx(font, text,
               { bounds.x + pad, bounds.y + bounds.height * 0.5f - fontSize * 0.45f },
               fontSize, 1.0f, UiTheme::TextDark());

    if (focused && caretBlink > 0.5f) {
        Vector2 sz = MeasureTextEx(font, text, fontSize, 1.0f);
        float cx = bounds.x + pad + sz.x + 2.0f;
        DrawRectangle((int)cx, (int)(bounds.y + 8), 2, (int)(bounds.height - 16), UiTheme::TextDark());
    }
}

void DrawSectionLabel(const Font& font, const char* text, Vector2 pos, float size) {
    DrawTextEx(font, text, pos, size, 1.0f, UiTheme::AccentGold());
}

void DrawStatusBanner(const Font& font, const char* text, bool isError) {
    if (!text || !text[0]) return;
    Rectangle bar = { 80, (float)VIRTUAL_HEIGHT - 70, (float)VIRTUAL_WIDTH - 160, 48 };
    Color bg = isError ? Fade(UiTheme::Danger(), 0.88f) : Fade(UiTheme::AccentWarm(), 0.88f);
    DrawRectangleRec(bar, bg);
    DrawRectangleLinesEx(bar, 1.5f, Fade(WHITE, 0.35f));
    Vector2 sz = MeasureTextEx(font, text, 18.0f, 1.0f);
    DrawTextEx(font, text,
               { bar.x + bar.width * 0.5f - sz.x * 0.5f,
                 bar.y + bar.height * 0.5f - sz.y * 0.5f },
               18.0f, 1.0f, WHITE);
}

void DrawBackHint(const Font& font, const char* text) {
    DrawTextEx(font, text, { 28, (float)VIRTUAL_HEIGHT - 36 }, 14.0f, 1.0f, UiTheme::TextMuted());
}

void DrawCenteredText(const Font& font, const char* text, float y, float size, Color color) {
    Vector2 sz = MeasureTextEx(font, text, size, 1.0f);
    DrawTextEx(font, text, { VIRTUAL_WIDTH * 0.5f - sz.x * 0.5f, y }, size, 1.0f, color);
}

float Approach(float current, float target, float speed, float dt) {
    float diff = target - current;
    float step = speed * dt;
    if (diff > step) return current + step;
    if (diff < -step) return current - step;
    return target;
}

Vector2 RemapMouseToVirtual() {
    Vector2 mouse = GetMousePosition();
    float scaleX = (float)VIRTUAL_WIDTH / (float)GetScreenWidth();
    float scaleY = (float)VIRTUAL_HEIGHT / (float)GetScreenHeight();
    return { mouse.x * scaleX, mouse.y * scaleY };
}

} // namespace Ui
