#ifndef SELECT_GALLERY_H
#define SELECT_GALLERY_H

#include <raylib.h>
#include <string>
#include <vector>
#include "raylib.h"
#include "ui_theme.h"

namespace Ui {

// Generic selectable gallery widget.
// Items are drawn as cards with a thumbnail texture, label, and lock state.
// Returns the clicked item id (or 0 if none).
// Items are expected to be contiguous ids starting from 1.
// area: rectangle where the gallery is drawn (horizontal row or vertical column? we'll do horizontal row for now)
// items: vector of Item structs, index 0 unused (id starts at 1)
// selectedId: currently selected id (1..N) or 0 for none
// mouse: current mouse position
// returns: clicked id (1..N) or 0
struct GalleryItem {
    Texture2D thumbnail; // can be 0 if not loaded (placeholder used)
    const char* label;
    bool locked; // if true, cannot be selected and appears dimmed
};

class SelectGallery {
public:
    static int DrawGallery(Rectangle area, const std::vector<GalleryItem>& items, int selectedId, const Font& font, Vector2 mouse);

private:
    static constexpr float kCardWidth = 100.0f;
    static constexpr float kCardHeight = 120.0f;
    static constexpr float kSpacing = 16.0f;
    static constexpr float kCornerRadius = 8.0f;
};

} // namespace Ui

#endif // SELECT_GALLERY_H