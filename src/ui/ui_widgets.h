#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <raylib.h>
#include <string>

namespace Ui {

struct GalleryItem {
    Texture2D thumbnail;
    const char* label;
    bool locked;
};

void DrawVignette(float strength = 0.55f);

void DrawGlassPanel(Rectangle bounds, float alpha = 1.0f);

// Returns true while hovered. hoverT is 0..1 (caller lerps).
bool DrawMenuButton(Rectangle bounds, const Font& font, const char* label,
                    float hoverT, bool primary = false, bool enabled = true);

bool DrawModeCard(Rectangle bounds, const Font& font,
                  const char* title, const char* subtitle, float hoverT);

void DrawTextField(Rectangle bounds, const Font& font, const char* text,
                   bool focused, float caretBlink);

void DrawSectionLabel(const Font& font, const char* text, Vector2 pos, float size = 16.0f);

void DrawStatusBanner(const Font& font, const char* text, bool isError);

void DrawBackHint(const Font& font, const char* text = "ESC  Back");

void DrawCenteredText(const Font& font, const char* text, float y, float size, Color color);

float Approach(float current, float target, float speed, float dt);

Vector2 RemapMouseToVirtual();

} // namespace Ui

#endif // UI_WIDGETS_H
