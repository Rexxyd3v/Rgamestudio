#include "select_gallery.h"
#include <raylib.h>

namespace Ui {

int SelectGallery::DrawGallery(Rectangle area, const std::vector<GalleryItem>& items, int selectedId, const Font& font, Vector2 mouse) {
    // Layout: horizontal row, centered vertically.
    float startX = area.x;
    float itemWidth = kCardWidth;
    float spacing = kSpacing;
    float totalWidth = (items.size() - 1) * (itemWidth + spacing) + itemWidth;
    float offsetX = (area.width - totalWidth) * 0.5f;
    if (offsetX < 0) offsetX = 0; // clamp to left if too wide

    int clickedId = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        const GalleryItem& item = items[i];
        int id = static_cast<int>(i) + 1; // ids start at 1
        bool isSelected = (selectedId == id);
        bool isHovered = false;

        float x = startX + offsetX + i * (itemWidth + spacing);
        float y = area.y + (area.height - kCardHeight) * 0.5f; // center vertically
        Rectangle cardRect = { x, y, kCardWidth, kCardHeight };

        // Check hover
        if (CheckCollisionPointRec(mouse, cardRect)) {
            isHovered = true;
        }

        // Draw card background
        Color bgColor = item.locked ? Fade(DARKGRAY, 0.7f) : Fade(GOLD, 0.1f);
        if (isSelected) {
            bgColor = Fade(GOLD, 0.2f);
        } else if (isHovered && !item.locked) {
            bgColor = Fade(GOLD, 0.15f);
        }
        DrawRectangleRounded(cardRect, kCornerRadius, 4, bgColor);


        // Draw thumbnail or placeholder
        Texture2D tex = item.thumbnail;
        if (tex.id == 0) {
            // placeholder: draw a gray rectangle with texture icon?
            DrawRectangleRounded({ x + 10, y + 10, kCardWidth - 20, kCardHeight - 40 }, kCornerRadius, 4, Fade(DARKGRAY, 0.5f));
            // maybe draw a lock icon if locked
            if (item.locked) {
                DrawText("LOCKED", x + 10, y + kCardHeight - 30, 10, Fade(WHITE, 0.7f));
            }
        } else {
            // Draw thumbnail centered
            float texScale = std::min((kCardWidth - 20) / tex.width, (kCardHeight - 40) / tex.height);
            float drawX = x + (kCardWidth - tex.width * texScale) * 0.5f;
            float drawY = y + 10;
            DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height },
                           { drawX, drawY, tex.width * texScale, tex.height * texScale },
                           { 0, 0 }, 0.0f, WHITE);
        }

        // Draw label below thumbnail
        float labelY = y + kCardHeight - 20;
        DrawTextEx(font, item.label, { x + 5, labelY }, font.baseSize * 0.8f, 1.0f, Fade(WHITE, 0.9f));

        // If locked, draw lock overlay
        if (item.locked) {
            // semi-transparent dark overlay
            DrawRectangleRounded(cardRect, kCornerRadius, 4, Fade(BLACK, 0.5f));

            // lock icon (simple)
            DrawText("LOCK", x + kCardWidth/2 - 20, y + kCardHeight/2 - 10, 20, Fade(WHITE, 0.8f));
        }

        // Handle click
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !item.locked) {
            clickedId = id;
        }
    }

    return clickedId;
}

} // namespace Ui